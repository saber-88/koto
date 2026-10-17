#include <koto/koto.hpp>
#include <string>
#include <vector>

using namespace koto;

int main()
{
    App app;

    TabsState tabs_state({
        "1. Code Editor",
        "2. File Browser",
        "3. System Telemetry",
        "4. Chess Board"
    });

    // Tab 1 Content: TextArea
    TextAreaState editor_state(
        "// Tab 1: Multi-line TextArea\n"
        "#include <koto/koto.hpp>\n\n"
        "int main() {\n"
        "    // Press Tab / Shift+Tab or 1..4 to switch tabs!\n"
        "    // Press 'T' to cycle Tab header styles (Bar, Pill, Underline).\n"
        "    return 0;\n"
        "}"
    );

    // Tab 2 Content: List (File browser)
    ListState files_state({
        "📁 src/",
        "  ├── app.cpp",
        "  ├── box.cpp",
        "  ├── frame.cpp",
        "  ├── layout.cpp",
        "  ├── list.cpp",
        "  ├── tabs.cpp",
        "  └── text_area.cpp",
        "📁 include/koto/",
        "  ├── tabs.hpp",
        "  └── text_area.hpp",
        "📄 CMakeLists.txt",
        "📄 README.md"
    });

    // Tab 3 Content: Telemetry (Gauge & Sparkline)
    std::vector<uint64_t> cpu_history = {12, 18, 25, 40, 65, 80, 55, 42, 60, 75, 90, 82, 70, 60, 48, 52};

    // Tab Style cycling
    TabsStyle current_style = TabsStyle::Underline;

    app.on_key([&](const KeyEvent& key) {
        if (key.is(Key::Escape) || (key.ctrl && (key.ch == 'C' || key.ch == 'c')))
        {
            app.quit();
            return;
        }

        // Direct number tabs
        if (!key.ctrl && !key.alt && key.ch >= '1' && key.ch <= '4')
        {
            tabs_state.set_selected(static_cast<size_t>(key.ch - '1'));
            return;
        }

        // 't' or 'T' cycles tab appearance
        if (!key.ctrl && !key.alt && (key.ch == 't' || key.ch == 'T'))
        {
            if (current_style == TabsStyle::Bar) current_style = TabsStyle::Pill;
            else if (current_style == TabsStyle::Pill) current_style = TabsStyle::Underline;
            else current_style = TabsStyle::Bar;
            return;
        }

        // F1 / F2 switches tabs without consuming Tab key inside editor
        if (key.is(Key::F1))
        {
            tabs_state.select_prev();
            return;
        }
        if (key.is(Key::F2))
        {
            tabs_state.select_next();
            return;
        }

        // Delegate keys according to active tab
        if (tabs_state.selected == 0)
        {
            editor_state.handle_event(key);
        }
        else if (tabs_state.selected == 1)
        {
            files_state.handle_event(key);
        }
    });

    app.on_render([&](Frame& frame) {
        Area screen = frame.area();
        if (!screen.is_valid())
        {
            return;
        }

        // Height needed for tabs: 2 rows for Underline, 1 row for Bar/Pill
        int tab_h = (current_style == TabsStyle::Underline) ? 2 : 1;

        auto chunks = Layout::vertical(screen, {
            Constraint::fixed(tab_h),
            Constraint::fill(),
            Constraint::fixed(1)
        });

        // 1. Draw Tabs Header
        Tabs(tabs_state)
            .style(current_style)
            .active_fg(Color::Black)
            .active_bg(Color::Cyan)
            .inactive_fg(Color::Gray)
            .separator_fg(Color::DarkGray)
            .draw(frame, chunks[0]);

        // 2. Draw Active Content Area based on tabs_state.selected
        Area content = chunks[1];
        switch (tabs_state.selected)
        {
            case 0: // Code Editor
            {
                TextArea(editor_state)
                    .border_style(BorderStyle::Rounded)
                    .title(" Code Editor ")
                    .line_numbers(true)
                    .focused(true)
                    .draw(frame, content);
                break;
            }
            case 1: // File Browser
            {
                List(files_state)
                    .border_style(BorderStyle::Rounded)
                    .title(" Workspace Files ")
                    .highlight_symbol("▶ ")
                    .highlight_fg(Color::Black)
                    .highlight_bg(Color::Emerald)
                    .draw(frame, content);
                break;
            }
            case 2: // System Telemetry
            {
                auto sub = Layout::vertical(content, {
                    Constraint::fixed(3),
                    Constraint::fixed(5),
                    Constraint::fill()
                });

                Box().border_style(BorderStyle::Rounded).title(" CPU Utilization (75%) ").draw(frame, sub[0]);
                Gauge()
                    .percent(75)
                    .filled_fg(Color::Amber)
                    .empty_bg(Color::DarkGray)
                    .use_fractional_blocks(true)
                    .draw(frame, sub[0].inset(1, 1, 1, 1));

                Box().border_style(BorderStyle::Rounded).title(" Load History ").draw(frame, sub[1]);
                Sparkline()
                    .data(cpu_history)
                    .fg(Color::Cyan)
                    .draw(frame, sub[1].inset(1, 1, 1, 1));
                break;
            }
            case 3: // Chess Board
            {
                Box().border_style(BorderStyle::Rounded).title(" Chess Board Grid ").draw(frame, content);
                Area board_area = content.inset(1, 1, 1, 1);
                auto grid = Layout::grid_uniform(board_area, 8, 8);

                for (size_t r = 0; r < 8; ++r)
                {
                    for (size_t c = 0; c < 8; ++c)
                    {
                        bool light = ((r + c) % 2 == 0);
                        uint32_t bg_col = light ? Color::Rgb(240, 217, 181) : Color::Rgb(181, 136, 99);
                        frame.fill(grid[r][c], U' ', Color::White, bg_col);
                    }
                }
                break;
            }
        }

        // 3. Status & Help Bar
        std::string help = " [1..4] Switch Tab  [F1/F2] Prev/Next Tab  [T] Toggle Style (Bar/Pill/Underline)  [Esc] Quit";
        Text(help)
            .fg(Color::White)
            .bg(Color::Rgb(30, 33, 40))
            .align(TextAlign::Left)
            .draw(frame, chunks[2]);
    });

    app.run();
    return 0;
}
