#include <koto/koto.hpp>
#include <string>
#include <vector>

using namespace koto;

int main()
{
    App app;
    FocusManager fm;

    // Bento Grid Panels
    TabsState nav_tabs({"Projects", "Search", "Git", "Debug", "Extensions"});
    ListState file_tree({"src/app.cpp", "src/focus.cpp", "src/frame.cpp", "include/koto/focus.hpp", "CMakeLists.txt"});
    file_tree.set_selected(1);

    TextAreaState editor_code(
        "// 2D Spatial Focus Navigation\n"
        "// Press Arrow Keys (Up/Down/Left/Right)!\n"
        "// Watch focus jump across asymmetrical bento panels."
    );

    TextInputState quick_cmd("echo 'Hello koto spatial navigation!'");

    const Id id_telemetry("panel_telemetry");
    const Id id_actions("panel_actions");

    // Register all elements and enable spatial 2D arrow navigation
    fm.navigation_policy(FocusNavPolicy::SpatialArrows)
      .add(nav_tabs)
      .add(file_tree)
      .add(editor_code)
      .add(quick_cmd)
      .add(id_telemetry)
      .add(id_actions);

    app.use_focus(fm);

    std::vector<uint64_t> telemetry_data = {15, 25, 40, 65, 80, 50, 70, 85, 95, 60, 45, 80, 75, 90};
    float gauge_val = 0.75f;

    app.on_key([&](const KeyEvent& key) {
        if (key.ctrl && (key.ch == 'q' || key.ch == 'Q')) {
            app.quit();
            return;
        }

        // When focused on stateless panels, space / enter can simulate activation
        if (fm.is_focused(id_actions) && key.is(Key::Enter)) {
            quick_cmd.set_text("ran action from palette");
        }
    });

    app.on_render([&](Frame& frame) {
        Area screen = frame.area();
        if (screen.width < 60 || screen.height < 20) {
            Box().title("Screen Too Small").draw(frame, screen);
            Text("Please enlarge your terminal window (min 60x20).").align(TextAlign::Center).draw(frame, Layout::center(screen, 46, 2));
            return;
        }

        // Layout:
        // Top: Nav Tabs (3)
        // Middle: 3 columns -> Left (25%), Center (45%), Right (30%)
        // Bottom: Command Input (3)
        // Footer: Status & Metrics (2)
        auto v_splits = Layout::vertical(screen, {
            Constraint::fixed(3),
            Constraint::fill(),
            Constraint::fixed(3),
            Constraint::fixed(2)
        });

        // 1. Top Panel: Navigation Tabs
        Tabs(nav_tabs)
            .border_style(BorderStyle::Rounded)
            .title(" [Panel A] Navigation Tabs ")
            .style(TabsStyle::Pill)
            .focused_border_fg(Color::Cyan)
            .draw(frame, v_splits[0]);

        // 2. Middle Row Bento Split
        auto middle_cols = Layout::horizontal(v_splits[1], {
            Constraint::percent(25), // Left Tree
            Constraint::percent(45), // Center Editor
            Constraint::fill()       // Right Telemetry & Actions
        });

        // Left: File Tree
        List(file_tree)
            .border_style(BorderStyle::Rounded)
            .title(" [Panel B] Explorer ")
            .highlight_symbol("→ ")
            .highlight_fg(Color::Black)
            .highlight_bg(Color::Cyan)
            .focused_border_fg(Color::Cyan)
            .draw(frame, middle_cols[0]);

        // Center: Code Editor
        TextArea(editor_code)
            .border_style(BorderStyle::Rounded)
            .title(" [Panel C] Code Editor ")
            .title_align(TitleAlign::Center)
            .line_numbers(true)
            .focused_border_fg(Color::Cyan)
            .draw(frame, middle_cols[1]);

        // Right: Split vertically into Telemetry (Top) and Actions (Bottom)
        auto right_rows = Layout::vertical(middle_cols[2], {
            Constraint::percent(50),
            Constraint::fill()
        });

        // Right Top: Telemetry Panel (Stateless Box with ID)
        bool tel_foc = fm.is_focused(id_telemetry);
        Box()
            .border_style(BorderStyle::Rounded)
            .title(" [Panel D] Telemetry ")
            .id(id_telemetry)
            .focused(tel_foc)
            .focused_border_fg(Color::Emerald)
            .filled(true)
            .draw(frame, right_rows[0]);

        Area tel_inner = right_rows[0].inset(1, 1, 1, 1);
        if (tel_inner.is_valid() && tel_inner.height >= 4) {
            auto tel_splits = Layout::vertical(tel_inner, {
                Constraint::fixed(1), // Label
                Constraint::fixed(1), // Gauge
                Constraint::fixed(1), // Sparkline label
                Constraint::fill()    // Sparkline
            });
            Text("CPU Utilization: 75%").fg(Color::LightGray).draw(frame, tel_splits[0]);
            Gauge().ratio(gauge_val).filled_fg(Color::Emerald).draw(frame, tel_splits[1]);
            Text("Activity History:").fg(Color::LightGray).draw(frame, tel_splits[2]);
            Sparkline().data(telemetry_data).fg(Color::Emerald).draw(frame, tel_splits[3]);
        }

        // Right Bottom: Action Palette (Stateless Box with ID)
        bool act_foc = fm.is_focused(id_actions);
        Box()
            .border_style(BorderStyle::Rounded)
            .title(" [Panel E] Quick Actions ")
            .id(id_actions)
            .focused(act_foc)
            .focused_border_fg(Color::Yellow)
            .filled(true)
            .draw(frame, right_rows[1]);

        Area act_inner = right_rows[1].inset(1, 2, 1, 2);
        if (act_inner.is_valid()) {
            std::string act_text = 
                "• [Enter] Run Command\n"
                "• [F2] Git Commit\n"
                "• [F5] Refresh Buffer\n"
                "• [Ctrl+Q] Quit";
            Text(act_text).fg(act_foc ? Color::Yellow : Color::DarkGray).draw(frame, act_inner);
        }

        // 3. Bottom Panel: Command Line Input
        TextInput(quick_cmd)
            .border_style(BorderStyle::Rounded)
            .title(" [Panel F] Terminal Prompt ")
            .focused_border_fg(Color::Cyan)
            .draw(frame, v_splits[2]);

        // 4. Metrics & Spatial Inspector Line
        std::string current_name;
        Area cur_bounds{0, 0, 0, 0};
        if (fm.is_focused(nav_tabs)) { current_name = "Panel A (Nav Tabs)"; cur_bounds = v_splits[0]; }
        else if (fm.is_focused(file_tree)) { current_name = "Panel B (Explorer)"; cur_bounds = middle_cols[0]; }
        else if (fm.is_focused(editor_code)) { current_name = "Panel C (Code Editor)"; cur_bounds = middle_cols[1]; }
        else if (fm.is_focused(id_telemetry)) { current_name = "Panel D (Telemetry)"; cur_bounds = right_rows[0]; }
        else if (fm.is_focused(id_actions)) { current_name = "Panel E (Quick Actions)"; cur_bounds = right_rows[1]; }
        else if (fm.is_focused(quick_cmd)) { current_name = "Panel F (Terminal Prompt)"; cur_bounds = v_splits[2]; }
        else { current_name = "None"; }

        std::string info_line = "Focused: " + current_name + 
                                "  │  Bounds: (r=" + std::to_string(cur_bounds.row) + 
                                ", c=" + std::to_string(cur_bounds.col) + 
                                ", w=" + std::to_string(cur_bounds.width) + 
                                ", h=" + std::to_string(cur_bounds.height) + 
                                ", Center=" + std::to_string(cur_bounds.center_row()) + "," + std::to_string(cur_bounds.center_col()) + ")";

        Text(info_line).fg(Color::Cyan).draw(frame, v_splits[3]);

        Text("Spatial 2D Nav: Use [↑ / ↓ / ← / →] to navigate spatially across 2D panels  •  [Tab] 1D Ring  •  [Click] Hit-Test")
            .fg(Color::DarkGray)
            .draw(frame, v_splits[3].set_row(v_splits[3].row + 1));
    });

    app.run();
    return 0;
}
