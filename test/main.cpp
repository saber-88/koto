#include <koto/koto.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace koto;

int main()
{
    App app;
    FocusManager fm;
    fm.navigation_policy(FocusNavPolicy::SpatialArrows);

    // -------------------------------------------------------------------------
    // Persistent State for Widgets
    // -------------------------------------------------------------------------

    // Tabs bar (Widget 1: Tabs)
    TabsState tabs_state({
        "1. Dashboard",
        "2. Editors & Inputs",
        "3. Containers & Styles",
        "4. Typography & Colors"
    });
    tabs_state.set_selected(0);

    // List of servers/nodes (Widget 2: List)
    ListState hosts({
        "● api-gateway-01       [RUNNING]",
        "● redis-primary-01     [RUNNING]",
        "▲ db-postgres-replica  [REINDEX]",
        "● auth-vault-service   [RUNNING]",
        "■ batch-worker-08      [IDLE]",
        "● telemetry-ingest-02  [RUNNING]",
        "● search-indexer-node  [RUNNING]"
    });
    hosts.set_selected(0);

    // Single-line text input (Widget 3: TextInput)
    TextInputState cmd_input("systemctl restart koto-daemon");

    // Multi-line code editor (Widget 4: TextArea)
    TextAreaState code_editor({
        "// Welcome to the koto v1.0.0 Master Showcase",
        "#include <koto/koto.hpp>",
        "using namespace koto;",
        "",
        "int main() {",
        "    App app;",
        "    ListState items({\"Alpha\", \"Beta\", \"Gamma\"});",
        "    app.on_render([&](Frame& frame) {",
        "        auto [l, r] = Layout::horizontal(frame.area(),",
        "            {Constraint::percent(35), Constraint::fill()});",
        "        List(items).title(\"Items\").draw(frame, l);",
        "        Box().title(\"Viewport\").draw(frame, r);",
        "    });",
        "    return app.run().has_value() ? 0 : 1;",
        "}"
    });

    // Telemetry and animation state (Widget 5: Sparkline, Widget 6: Gauge)
    std::vector<uint64_t> load_history = {
        15, 22, 35, 48, 62, 70, 65, 80, 92, 85, 78, 60, 
        52, 68, 75, 88, 95, 82, 70, 64, 58, 72, 84, 91
    };
    int cpu_percent = 64;
    int gauge_dir = 1;
    uint64_t tick_count = 0;
    std::string status_msg = "Ready. Use [Tab] / [Arrows] to navigate, [1-4] for Tabs, [M] for Modal.";

    // Modal dialog state (Widget 11: Clear, Widget 12: Fill)
    bool show_modal = false;

    // Register primary states with FocusManager
    fm.add(tabs_state, Id("tabs_nav"));
    fm.add(hosts, Id("hosts_list"));
    fm.add(cmd_input, Id("cmd_input"));
    fm.add(code_editor, Id("code_editor"));

    auto update_focus_for_tab = [&](size_t tab_idx) {
        fm.set_enabled(Id("hosts_list"), tab_idx == 0);
        fm.set_enabled(Id("cmd_input"), tab_idx == 1);
        fm.set_enabled(Id("code_editor"), tab_idx == 1);

        if (tab_idx == 0) fm.focus(Id("hosts_list"));
        else if (tab_idx == 1) fm.focus(Id("cmd_input"));
        else if (tab_idx == 2) fm.focus(Id("tabs_nav"));
        else if (tab_idx == 3) fm.focus(Id("tabs_nav"));
    };
    update_focus_for_tab(0);

    auto open_modal = [&]() {
        if (!show_modal)
        {
            show_modal = true;
            fm.push_scope(Id("demo_modal"), ScopeMode::Modal);
            status_msg = "Modal opened: focus is strictly confined to dialog controls.";
        }
    };

    auto close_modal = [&](std::string_view reason) {
        if (show_modal)
        {
            fm.pop_scope();
            show_modal = false;
            status_msg = std::string("Modal closed: ") + std::string(reason);
        }
    };

    tabs_state.on_change = [&](size_t new_idx, const std::string& title) {
        (void)title;
        update_focus_for_tab(new_idx);
        status_msg = "Switched to tab " + std::to_string(new_idx + 1);
    };

    app.use_focus(fm);

    // Global Key Handlers
    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'q' || key.ch == 'Q' || (key.ctrl && (key.ch == 'c' || key.ch == 'C')))
        {
            app.quit();
            return;
        }
        if (key.is(Key::Escape) && show_modal)
        {
            close_modal("Dismissed with Escape");
            return;
        }
        if ((key.ch == 'm' || key.ch == 'M') && !show_modal && !code_editor.focused && !cmd_input.focused)
        {
            open_modal();
            return;
        }
        if (!show_modal && !cmd_input.focused && !code_editor.focused && key.ch >= '1' && key.ch <= '4')
        {
            size_t idx = static_cast<size_t>(key.ch - '1');
            tabs_state.set_selected(idx);
            update_focus_for_tab(idx);
            status_msg = "Switched to Tab " + std::to_string(idx + 1);
            return;
        }
    });

    // -------------------------------------------------------------------------
    // Main Render Loop
    // -------------------------------------------------------------------------
    app.on_render([&](Frame& frame) {
        Area screen = frame.area();
        if (!screen.is_valid()) return;

        // Animate metrics periodically
        tick_count++;
        if (tick_count % 12 == 0)
        {
            cpu_percent += (gauge_dir * 2);
            if (cpu_percent >= 94) gauge_dir = -1;
            if (cpu_percent <= 28) gauge_dir = 1;

            uint64_t next_load = static_cast<uint64_t>(30 + (cpu_percent / 2) + static_cast<int>(tick_count % 15));
            if (!load_history.empty())
            {
                load_history.erase(load_history.begin());
            }
            load_history.push_back(next_load);
        }

        // Layout: Top Header (fixed 3), Tabs (fixed 3), Body (fill), Footer (fixed 3)
        auto [header_area, tabs_area, body_area, footer_area] = Layout::vertical(
            screen,
            {Constraint::fixed(3), Constraint::fixed(3), Constraint::fill(), Constraint::fixed(3)}
        );

        // 1. Header (Widget: Box, Widget: Text)
        Box()
            .border_style(BorderStyle::Rounded)
            .draw(frame, header_area);
        Text(" koto v1.0.0 Master Showcase — 14 Widgets in Pure C++23 ")
            .fg(Color::Cyan)
            .bold()
            .align(TextAlign::Center)
            .draw(frame, header_area.inset(1, 1, 1, 1));

        // 2. Tabs Bar (Widget: Tabs)
        Tabs(tabs_state)
            .style(TabsStyle::Pill)
            .active_fg(Color::Emerald)
            .active_bg(Color::rgb(20, 45, 30))
            .inactive_fg(Color::LightGray)
            .border_style(BorderStyle::Rounded)
            .draw(frame, tabs_area);

        // 3. Tab Body
        size_t cur_tab = tabs_state.selected;

        if (cur_tab == 0)
        {
            // -----------------------------------------------------------------
            // TAB 1: Dashboard (List, Gauge, Sparkline, Button, Rule)
            // -----------------------------------------------------------------
            auto [left_col, right_col] = Layout::horizontal(
                body_area,
                {Constraint::percent(35), Constraint::fill()}
            );

            // Left: Node List (Widget: List)
            List(hosts)
                .title(" Cluster Nodes ")
                .title_align(TitleAlign::Left)
                .highlight_symbol("> ")
                .highlight_fg(Color::Emerald)
                .highlight_bg(Color::rgb(25, 45, 35))
                .border_style(BorderStyle::Rounded)
                .draw(frame, left_col);

            // Right: Telemetry stack
            auto [cpu_box_area, spark_box_area, actions_area] = Layout::vertical(
                right_col,
                {Constraint::fixed(4), Constraint::fill(), Constraint::fixed(5)}
            );

            // CPU Gauge (Widget: Gauge)
            Box()
                .title(" CPU Utilization ")
                .border_style(BorderStyle::Rounded)
                .draw(frame, cpu_box_area);
            std::string cpu_label = std::to_string(cpu_percent) + "% Load";
            Gauge()
                .percent(cpu_percent)
                .label(cpu_label)
                .filled_fg(cpu_percent > 85 ? Color::Red : (cpu_percent > 70 ? Color::Amber : Color::Emerald))
                .label_fg(Color::White)
                .use_fractional_blocks(true)
                .draw(frame, cpu_box_area.inset(1, 2, 1, 2));

            // Sparkline (Widget: Sparkline)
            Box()
                .title(" Network Throughput (MB/s) ")
                .border_style(BorderStyle::Rounded)
                .draw(frame, spark_box_area);
            Sparkline()
                .data(load_history)
                .max(100)
                .fg(Color::Cyan)
                .align_right(true)
                .draw(frame, spark_box_area.inset(1, 2, 1, 2));

            // Actions & Rule (Widget: Rule, Widget: Button)
            auto [rule_area, btn_row] = Layout::vertical(
                actions_area,
                {Constraint::fixed(1), Constraint::fill()}
            );
            Rule(" Quick Actions ")
                .style(BorderStyle::Single)
                .title_fg(Color::LightGray)
                .draw(frame, rule_area);

            auto [b1, b2, b3] = Layout::horizontal(
                btn_row,
                {Constraint::percent(33), Constraint::percent(33), Constraint::fill()}
            );

            Button(" Open Modal [M] ")
                .id(Id("btn_modal"))
                .focus(fm)
                .focused_border_fg(Color::Amber)
                .on_click([&]() { open_modal(); })
                .draw(frame, b1.inset(0, 1, 0, 1));

            Button(" Ping Cluster ")
                .id(Id("btn_alert"))
                .focus(fm)
                .focused_border_fg(Color::Cyan)
                .on_click([&]() { status_msg = "Ping: all 7 nodes responded in 1.4ms."; })
                .draw(frame, b2.inset(0, 1, 0, 1));

            Button(" Reset Metrics ")
                .id(Id("btn_reset"))
                .focus(fm)
                .focused_border_fg(Color::Emerald)
                .on_click([&]() {
                    cpu_percent = 50;
                    std::fill(load_history.begin(), load_history.end(), 50);
                    status_msg = "Metrics reset to nominal baselines.";
                })
                .draw(frame, b3.inset(0, 1, 0, 1));
        }
        else if (cur_tab == 1)
        {
            // -----------------------------------------------------------------
            // TAB 2: Editors & Inputs (TextInput, TextArea, Text)
            // -----------------------------------------------------------------
            auto [top_row, bottom_row] = Layout::vertical(
                body_area,
                {Constraint::percent(60), Constraint::fill()}
            );

            auto [input_col, editor_col] = Layout::horizontal(
                top_row,
                {Constraint::percent(45), Constraint::fill()}
            );

            // Command Input (Widget: TextInput)
            auto [text_in_area, echo_area] = Layout::vertical(
                input_col,
                {Constraint::fixed(5), Constraint::fill()}
            );

            TextInput(cmd_input)
                .title(" Interactive Command Input ")
                .placeholder("Type shell command...")
                .border_style(BorderStyle::Rounded)
                .focused_border_fg(Color::Cyan)
                .draw(frame, text_in_area);

            Box()
                .title(" Input Stream Echo ")
                .border_style(BorderStyle::Single)
                .draw(frame, echo_area);
            Text("Value: \"" + cmd_input.value + "\"\nLength: " + std::to_string(cmd_input.value.size()) + " bytes\nCursor: " + std::to_string(cmd_input.cursor))
                .fg(Color::Emerald)
                .draw(frame, echo_area.inset(1, 2, 1, 2));

            // Code Editor (Widget: TextArea)
            TextArea(code_editor)
                .title(" Multi-Line Code Editor (C++23) ")
                .border_style(BorderStyle::Rounded)
                .line_numbers(true)
                .focused_border_fg(Color::Cyan)
                .draw(frame, editor_col);

            // Bottom documentation hint (Widget: Text)
            Box()
                .title(" Input System Architecture ")
                .border_style(BorderStyle::Rounded)
                .draw(frame, bottom_row);
            Text("• TextInput & TextArea support full UTF-8 grapheme cursor navigation, CJK stepping, and IME hardware synchronization.\n"
                 "• Backspace, Delete, Home/End, and word-skipping are handled automatically.\n"
                 "• Bracketed paste streams isolate multi-megabyte clipboard pastes atomically without freezing.")
                .fg(Color::LightGray)
                .draw(frame, bottom_row.inset(1, 2, 1, 2));
        }
        else if (cur_tab == 2)
        {
            // -----------------------------------------------------------------
            // TAB 3: Containers & Styles (Vbox, Hbox, Box BorderStyles, Fill)
            // -----------------------------------------------------------------
            auto [cards_row, canvas_row] = Layout::vertical(
                body_area,
                {Constraint::percent(50), Constraint::fill()}
            );

            // Four Border Styles in an Hbox (Widget: Hbox)
            Hbox border_showcase;
            border_showcase.add<Box>(Constraint::percent(25))
                .border_style(BorderStyle::Single)
                .title(" Single ");
            border_showcase.add<Box>(Constraint::percent(25))
                .border_style(BorderStyle::Double)
                .title(" Double ")
                .fg(Color::Cyan);
            border_showcase.add<Box>(Constraint::percent(25))
                .border_style(BorderStyle::Rounded)
                .title(" Rounded ")
                .fg(Color::Emerald);
            border_showcase.add<Box>(Constraint::percent(25))
                .border_style(BorderStyle::Thick)
                .title(" Thick ")
                .fg(Color::Amber);
            border_showcase.draw(frame, cards_row);

            // Add inner text into each card
            auto [c1, c2, c3, c4] = Layout::horizontal(
                cards_row,
                {Constraint::percent(25), Constraint::percent(25), Constraint::percent(25), Constraint::fill()}
            );
            Text("Single line border\nClean & minimal style").draw(frame, c1.inset(1, 2, 1, 2));
            Text("Double line border\nClassic high-visibility").draw(frame, c2.inset(1, 2, 1, 2));
            Text("Rounded border\nSmooth modern aesthetic").draw(frame, c3.inset(1, 2, 1, 2));
            Text("Thick line border\nHeavy structural framing").draw(frame, c4.inset(1, 2, 1, 2));

            // Background Fill Demonstration (Widget: Fill, Widget: Vbox)
            Box()
                .title(" Layered Composite Containers & Background Fill ")
                .border_style(BorderStyle::Rounded)
                .draw(frame, canvas_row);

            Area inner_canvas = canvas_row.inset(1, 2, 1, 2);
            // Fill background with subtle pattern
            Fill(Color::rgb(22, 28, 38))
                .char_code(U'·')
                .fg(Color::rgb(60, 75, 95))
                .draw(frame, inner_canvas);

            // Layered floating box on top of the fill
            Area floating_box = Layout::center(inner_canvas, 48, 5);
            Box()
                .border_style(BorderStyle::Rounded)
                .fg(Color::Cyan)
                .bg(Color::rgb(15, 20, 28))
                .title(" Floating Card ")
                .draw(frame, floating_box);
            Text("This panel rests on top of a patterned Fill canvas.\nkoto diffs every cell to render tear-free.")
                .align(TextAlign::Center)
                .draw(frame, floating_box.inset(1, 1, 1, 1));
        }
        else if (cur_tab == 3)
        {
            // -----------------------------------------------------------------
            // TAB 4: Typography & Colors (Text styling, colors, word-wrap)
            // -----------------------------------------------------------------
            auto [palette_row, text_row] = Layout::vertical(
                body_area,
                {Constraint::fixed(4), Constraint::fill()}
            );

            // Color Palette
            Box()
                .title(" 24-bit TrueColor Palette ")
                .border_style(BorderStyle::Rounded)
                .draw(frame, palette_row);

            auto chips = Layout::horizontal(
                palette_row.inset(1, 2, 1, 2),
                {
                    Constraint::percent(12), Constraint::percent(12),
                    Constraint::percent(12), Constraint::percent(12),
                    Constraint::percent(12), Constraint::percent(12),
                    Constraint::percent(12), Constraint::fill()
                }
            );

            Text("■ Cyan").fg(Color::Cyan).draw(frame, chips[0]);
            Text("■ Emerald").fg(Color::Emerald).draw(frame, chips[1]);
            Text("■ Amber").fg(Color::Amber).draw(frame, chips[2]);
            Text("■ Orange").fg(Color::Orange).draw(frame, chips[3]);
            Text("■ Magenta").fg(Color::Magenta).draw(frame, chips[4]);
            Text("■ Yellow").fg(Color::Yellow).draw(frame, chips[5]);
            Text("■ Blue").fg(Color::Blue).draw(frame, chips[6]);
            Text("■ Red").fg(Color::Red).draw(frame, chips[7]);

            // Typography Attributes & Word Wrapping
            auto [styles_col, wrap_col] = Layout::horizontal(
                text_row,
                {Constraint::percent(40), Constraint::fill()}
            );

            Box()
                .title(" Text Styles & Attributes ")
                .border_style(BorderStyle::Rounded)
                .draw(frame, styles_col);

            auto style_items = Layout::vertical(
                styles_col.inset(1, 2, 1, 2),
                {
                    Constraint::fixed(1), Constraint::fixed(1),
                    Constraint::fixed(1), Constraint::fixed(1),
                    Constraint::fixed(1), Constraint::fixed(1)
                }
            );

            Text("Standard plain text label").draw(frame, style_items[0]);
            Text("Bold high-emphasis text").bold().fg(Color::White).draw(frame, style_items[1]);
            Text("Dim secondary metadata text").dim().draw(frame, style_items[2]);
            Text("Italic citation text").italic().fg(Color::Yellow).draw(frame, style_items[3]);
            Text("Underlined actionable hyperlink").underline().fg(Color::Cyan).draw(frame, style_items[4]);
            Text(" Inverted reverse badge ").reverse().fg(Color::Emerald).draw(frame, style_items[5]);

            Box()
                .title(" Dynamic Word Wrapping (TextWrap::Wrap) ")
                .border_style(BorderStyle::Rounded)
                .draw(frame, wrap_col);

            Text(
                "koto features an intelligent Unicode-aware word wrapping algorithm. "
                "Instead of abruptly truncating sentences or breaking in the middle of words, "
                "it measures grapheme clusters, respects East Asian Width (UAX-11) for double-width "
                "CJK characters, and breaks cleanly at whitespace boundaries.\n\n"
                "Resize your terminal emulator horizontally or vertically to see flex constraints "
                "and word wrapping adapt smoothly in real time!"
            )
            .wrap()
            .fg(Color::LightGray)
            .draw(frame, wrap_col.inset(1, 2, 1, 2));
        }

        // 4. Modal Dialog Overlay (Widget: Clear, Widget: Box, Widget: Button)
        if (show_modal)
        {
            Area modal_box = Layout::center(screen, 54, 11);
            if (modal_box.is_valid())
            {
                // Drop shadow
                Area shadow = modal_box.inset(0, 0, -1, -2);
                Fill(Color::rgb(10, 10, 15)).char_code(U'▒').draw(frame, shadow);

                // Erase background cells cleanly
                Clear().draw(frame, modal_box);

                Box()
                    .title(" Modal Focus Trap Demo ")
                    .title_align(TitleAlign::Center)
                    .border_style(BorderStyle::Double)
                    .fg(Color::Amber)
                    .bg(Color::rgb(15, 20, 30))
                    .draw(frame, modal_box);

                Area inner_modal = modal_box.inset(1, 3, 1, 3);
                auto [m_msg, m_actions] = Layout::vertical(
                    inner_modal,
                    {Constraint::fill(), Constraint::fixed(3)}
                );

                Text("Focus is strictly confined to this modal dialog!\n"
                     "Background widgets are shielded from mouse & keys.\n"
                     "Press [Tab] to cycle buttons, or [Esc] to exit.")
                    .fg(Color::White)
                    .draw(frame, m_msg);

                auto [m_b1, m_spacer, m_b2] = Layout::horizontal(
                    m_actions,
                    {Constraint::fixed(18), Constraint::fill(), Constraint::fixed(18)}
                );

                Button("  Confirm Action  ")
                    .id(Id("modal_confirm"))
                    .focus(fm)
                    .focused_border_fg(Color::Emerald)
                    .on_click([&]() { close_modal("Confirmed"); })
                    .draw(frame, m_b1);

                Button("   Dismiss [Esc]  ")
                    .id(Id("modal_cancel"))
                    .focus(fm)
                    .focused_border_fg(Color::Red)
                    .on_click([&]() { close_modal("Dismissed"); })
                    .draw(frame, m_b2);
            }
        }

        // 5. Global Footer / Status Bar
        Box()
            .border_style(BorderStyle::Single)
            .fg(Color::DarkGray)
            .draw(frame, footer_area);

        auto [status_col, hints_col] = Layout::horizontal(
            footer_area.inset(0, 1, 0, 1),
            {Constraint::percent(45), Constraint::fill()}
        );

        Text(" " + status_msg)
            .fg(Color::Emerald)
            .draw(frame, status_col);

        Text("[Tab] Focus | [1-4] Tabs | [M] Modal | [Q] Quit ")
            .fg(Color::LightGray)
            .align(TextAlign::Right)
            .draw(frame, hints_col);
    });

    auto res = app.run();
    if (!res)
    {
        std::fprintf(stderr, "App error: %.*s (errno: %d)\n",
                     static_cast<int>(res.error().message().size()),
                     res.error().message().data(),
                     res.error().system_errno);
        return 1;
    }
    return 0;
}
