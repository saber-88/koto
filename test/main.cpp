#include <koto/koto.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
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
        "        List(items).id(Id(\"items\")).title(\"Items\").draw(frame, l);",
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
    double anim_time = 0.0;
    double spark_timer = 0.0;
    double marquee_timer = 0.0;
    size_t marquee_offset = 0;
    double rolling_fps = 60.0;
    size_t fill_glyph_idx = 0;
    bool invert_palette = false;

    std::string status_msg = "Ready. Use [Tab] / [Arrows] or Mouse to navigate.";
    std::string mouse_status = "Mouse: Click any widget to shift focus";

    // Modal dialog state (Widget 11: Clear, Widget 12: Fill)
    bool show_modal = false;

    // Register primary states with FocusManager using identical IDs
    fm.add(tabs_state, Id("tabs_nav"));
    fm.add(hosts, Id("hosts_list"));
    fm.add(cmd_input, Id("cmd_input"));
    fm.add(code_editor, Id("code_editor"));

    auto update_focus_for_tab = [&](size_t tab_idx) {
        fm.set_enabled(Id("hosts_list"), tab_idx == 0);
        fm.set_enabled(Id("cmd_input"), tab_idx == 1);
        fm.set_enabled(Id("code_editor"), tab_idx == 1);
        fm.set_enabled(Id("btn_cycle_fill"), tab_idx == 2);
        fm.set_enabled(Id("btn_accent"), tab_idx == 3);

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
            status_msg = "Modal opened: background shielded; focus trapped inside dialog.";
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

    hosts.on_change = [&](int idx, const std::string& item) {
        (void)item;
        status_msg = "Selected cluster node #" + std::to_string(idx + 1);
    };

    app.use_focus(fm);

    // -------------------------------------------------------------------------
    // Mouse Event Reporting & Hit Tracking
    // -------------------------------------------------------------------------
    app.on_mouse([&](const MouseEvent& mouse) {
        if (mouse.action == MouseAction::Press)
        {
            std::string btn_name = (mouse.button == MouseButton::Left ? "Left" :
                                   (mouse.button == MouseButton::Right ? "Right" :
                                   (mouse.button == MouseButton::WheelUp ? "WheelUp" :
                                   (mouse.button == MouseButton::WheelDown ? "WheelDown" : "Other"))));

            std::string target_name = "Background";
            if (mouse.target == Id("tabs_nav")) target_name = "TabsBar";
            else if (mouse.target == Id("hosts_list")) target_name = "ClusterNodes";
            else if (mouse.target == Id("cmd_input")) target_name = "CommandInput";
            else if (mouse.target == Id("code_editor")) target_name = "CodeEditor";
            else if (mouse.target == Id("btn_modal")) target_name = "Button[Modal]";
            else if (mouse.target == Id("btn_alert")) target_name = "Button[Ping]";
            else if (mouse.target == Id("btn_reset")) target_name = "Button[Reset]";
            else if (mouse.target == Id("btn_cycle_fill")) target_name = "Button[CycleFill]";
            else if (mouse.target == Id("btn_accent")) target_name = "Button[Accent]";
            else if (mouse.target == Id("modal_confirm")) target_name = "Modal[Confirm]";
            else if (mouse.target == Id("modal_cancel")) target_name = "Modal[Cancel]";
            else if (!mouse.target.is_none()) target_name = "Id(" + std::to_string(mouse.target.value()) + ")";

            mouse_status = "Mouse: " + btn_name + " @ (" + std::to_string(mouse.row) + "," +
                           std::to_string(mouse.col) + ") -> [" + target_name + "]";
        }
    });

    // -------------------------------------------------------------------------
    // Animation Tick Engine (~60 FPS continuous update loop)
    // -------------------------------------------------------------------------
    app.on_tick([&](double dt) {
        anim_time += dt;

        // Rolling FPS estimation
        rolling_fps = 0.95 * rolling_fps + 0.05 * (dt > 0.0001 ? (1.0 / dt) : 60.0);

        // 1. Continuous smooth CPU utilization wave
        double wave = 58.0 + 30.0 * std::sin(anim_time * 1.8) + 8.0 * std::cos(anim_time * 4.2);
        cpu_percent = std::clamp(static_cast<int>(wave), 8, 98);

        // 2. Continuous flowing Sparkline telemetry (~12 samples/sec)
        spark_timer += dt;
        if (spark_timer >= 0.08)
        {
            spark_timer = 0.0;
            double sample = 52.0 + 36.0 * std::sin(anim_time * 2.2) + 10.0 * std::cos(anim_time * 5.4);
            uint64_t val = static_cast<uint64_t>(std::clamp(static_cast<int>(sample), 10, 100));
            if (load_history.size() >= 36)
            {
                load_history.erase(load_history.begin());
            }
            load_history.push_back(val);
        }

        // 3. Smooth Marquee text scrolling (~12 steps/sec)
        marquee_timer += dt;
        if (marquee_timer >= 0.08)
        {
            marquee_timer = 0.0;
            marquee_offset++;
        }
    });

    // -------------------------------------------------------------------------
    // Global Keyboard Shortcuts
    // -------------------------------------------------------------------------
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

        // Layout: Top Header (fixed 3), Tabs (fixed 3), Body (fill), Footer (fixed 3)
        auto [header_area, tabs_area, body_area, footer_area] = Layout::vertical(
            screen,
            {Constraint::fixed(3), Constraint::fixed(3), Constraint::fill(), Constraint::fixed(3)}
        );

        // 1. Header (Widget: Box, Widget: Text) with Braille Spinner & Live Clock
        static constexpr std::array<const char*, 10> spinner_frames = {
            "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"
        };
        size_t spinner_idx = static_cast<size_t>(anim_time * 12.0) % spinner_frames.size();
        int mins = static_cast<int>(anim_time) / 60;
        int secs = static_cast<int>(anim_time) % 60;
        int tenths = static_cast<int>(anim_time * 10.0) % 10;

        char time_buf[16];
        std::snprintf(time_buf, sizeof(time_buf), "%02d:%02d.%d", mins, secs, tenths);
        char fps_buf[16];
        std::snprintf(fps_buf, sizeof(fps_buf), "%.1f FPS", rolling_fps);

        Box()
            .border_style(BorderStyle::Rounded)
            .fg(Color::Cyan)
            .draw(frame, header_area);

        auto [title_col, live_col] = Layout::horizontal(
            header_area.inset(1, 2, 1, 2),
            {Constraint::fill(), Constraint::fixed(28)}
        );

        std::string header_title = std::string(spinner_frames[spinner_idx]) +
                                   " koto v1.0.0 Master Showcase • 14 Widgets in Pure C++23";
        Text(header_title)
            .fg(Color::Cyan)
            .bold()
            .draw(frame, title_col);

        // Pulsing live indicator dot
        uint8_t live_g = static_cast<uint8_t>(180 + static_cast<int>(60.0 * std::sin(anim_time * 4.0)));
        uint32_t live_dot_col = Color::rgb(40, live_g, 100);

        std::string live_str = "● LIVE " + std::string(time_buf) + " | " + std::string(fps_buf);
        Text(live_str)
            .fg(live_dot_col)
            .bold()
            .align(TextAlign::Right)
            .draw(frame, live_col);

        // 2. Tabs Bar (Widget 1: Tabs) - explicitly tagged with Id("tabs_nav")
        Tabs(tabs_state)
            .id(Id("tabs_nav"))
            .style(TabsStyle::Pill)
            .active_fg(Color::Emerald)
            .active_bg(Color::rgb(20, 45, 30))
            .inactive_fg(Color::LightGray)
            .border_style(BorderStyle::Rounded)
            .focused_border_fg(Color::Emerald)
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

            // Left: Node List (Widget 2: List) - explicitly tagged with Id("hosts_list")
            List(hosts)
                .id(Id("hosts_list"))
                .title(" Cluster Nodes [Click / Arrows] ")
                .title_align(TitleAlign::Left)
                .highlight_symbol("> ")
                .highlight_fg(Color::Emerald)
                .highlight_bg(Color::rgb(25, 45, 35))
                .border_style(BorderStyle::Rounded)
                .focused_border_fg(Color::Cyan)
                .draw(frame, left_col);

            // Right: Telemetry stack
            auto [cpu_box_area, spark_box_area, actions_area] = Layout::vertical(
                right_col,
                {Constraint::fixed(4), Constraint::fill(), Constraint::fixed(5)}
            );

            // CPU Gauge (Widget 6: Gauge) - Smooth dynamic harmonic animation
            Box()
                .title(" Dynamic CPU Utilization (Real-Time Sub-Cell) ")
                .border_style(BorderStyle::Rounded)
                .draw(frame, cpu_box_area);

            std::string cpu_label = std::to_string(cpu_percent) + "% Load";
            uint32_t gauge_col = cpu_percent > 85 ? Color::Red : (cpu_percent > 65 ? Color::Amber : Color::Emerald);
            Gauge()
                .percent(cpu_percent)
                .label(cpu_label)
                .filled_fg(gauge_col)
                .label_fg(Color::White)
                .use_fractional_blocks(true)
                .draw(frame, cpu_box_area.inset(1, 2, 1, 2));

            // Sparkline (Widget 9: Sparkline) - Streaming telemetry wave
            Box()
                .title(" Live Network Throughput (MB/s) [Flowing 60 FPS] ")
                .border_style(BorderStyle::Rounded)
                .draw(frame, spark_box_area);

            Area spark_inner = spark_box_area.inset(1, 2, 1, 2);
            auto [spark_view, ecg_view] = Layout::vertical(
                spark_inner,
                {Constraint::fill(), Constraint::fixed(1)}
            );

            Sparkline()
                .data(load_history)
                .max(100)
                .fg(Color::Cyan)
                .align_right(true)
                .draw(frame, spark_view);

            // Animated scanning ECG pulse line
            int scan_pos = static_cast<int>(anim_time * 16.0) % std::max(1, ecg_view.width);
            std::string ecg_str;
            ecg_str.reserve(static_cast<size_t>(ecg_view.width));
            for (int col = 0; col < ecg_view.width; ++col)
            {
                int dist = col - scan_pos;
                if (dist == 0) ecg_str += "▲";
                else if (dist == 1 || dist == -1) ecg_str += "─";
                else if (dist == 2) ecg_str += "▼";
                else ecg_str += "─";
            }
            Text("Pulse: " + ecg_str)
                .fg(Color::Emerald)
                .dim()
                .draw(frame, ecg_view);

            // Actions & Rule (Widget 8: Rule, Widget 2: Button)
            auto [rule_area, btn_row] = Layout::vertical(
                actions_area,
                {Constraint::fixed(1), Constraint::fill()}
            );
            Rule(" Quick Actions (Click with Mouse or Tab to focus) ")
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
                .on_click([&]() { status_msg = "Ping: all 7 nodes responded in 1.2ms latency."; })
                .draw(frame, b2.inset(0, 1, 0, 1));

            Button(" Reset Metrics ")
                .id(Id("btn_reset"))
                .focus(fm)
                .focused_border_fg(Color::Emerald)
                .on_click([&]() {
                    cpu_percent = 50;
                    std::fill(load_history.begin(), load_history.end(), 50);
                    status_msg = "Metrics reset to 50% baseline.";
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

            // Command Input (Widget 13: TextInput) - explicitly tagged with Id("cmd_input")
            auto [text_in_area, echo_area] = Layout::vertical(
                input_col,
                {Constraint::fixed(5), Constraint::fill()}
            );

            TextInput(cmd_input)
                .id(Id("cmd_input"))
                .title(" Interactive Command Input [Click to Focus] ")
                .placeholder("Type shell command...")
                .border_style(BorderStyle::Rounded)
                .focused_border_fg(Color::Cyan)
                .draw(frame, text_in_area);

            Box()
                .title(" Input Stream Echo & Caret Pulse ")
                .border_style(BorderStyle::Single)
                .draw(frame, echo_area);

            bool caret_on = (static_cast<int>(anim_time * 2.5) % 2) == 0;
            std::string caret_str = caret_on ? "█" : " ";
            std::string focus_label = cmd_input.focused ? "[ACTIVE FOCUS]" : "[UNFOCUSED]";

            Text("Value:    \"" + cmd_input.value + "\"" + caret_str + "\n"
                 "Length:   " + std::to_string(cmd_input.value.size()) + " bytes\n"
                 "Cursor:   column " + std::to_string(cmd_input.cursor) + "\n"
                 "Status:   " + focus_label + "\n"
                 "Protocol: UTF-8 Grapheme Cluster Engine")
                .fg(Color::Emerald)
                .draw(frame, echo_area.inset(1, 2, 1, 2));

            // Code Editor (Widget 12: TextArea) - explicitly tagged with Id("code_editor")
            TextArea(code_editor)
                .id(Id("code_editor"))
                .title(" Multi-Line Code Editor (C++23) [Click / Wheel] ")
                .border_style(BorderStyle::Rounded)
                .line_numbers(true)
                .focused_border_fg(Color::Cyan)
                .draw(frame, editor_col);

            // Bottom architectural documentation (Widget 11: Text)
            Box()
                .title(" Input System Architecture ")
                .border_style(BorderStyle::Rounded)
                .draw(frame, bottom_row);
            Text("• TextInput & TextArea support full UTF-8 grapheme cursor navigation, CJK stepping, and IME synchronization.\n"
                 "• Mouse clicks place focus directly onto the editor or input line; mouse wheel scrolls lines smoothly.\n"
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

            // Split top row between Hbox and Vbox showcases
            auto [hbox_area, vbox_area] = Layout::horizontal(
                cards_row,
                {Constraint::percent(65), Constraint::fill()}
            );

            // 1. Horizontal Border Styles in an Hbox (Widget 6: Hbox)
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
            border_showcase.draw(frame, hbox_area);

            // Add inner text into each Hbox card
            auto [c1, c2, c3, c4] = Layout::horizontal(
                hbox_area,
                {Constraint::percent(25), Constraint::percent(25), Constraint::percent(25), Constraint::fill()}
            );
            Text("Single border\nClean & minimal").draw(frame, c1.inset(1, 1, 1, 1));
            Text("Double border\nHigh visibility").draw(frame, c2.inset(1, 1, 1, 1));
            Text("Rounded border\nSmooth modern").draw(frame, c3.inset(1, 1, 1, 1));
            Text("Thick border\nHeavy structural").draw(frame, c4.inset(1, 1, 1, 1));

            // 2. Vertical Composite Container (Widget 14: Vbox)
            Vbox vbox_showcase;
            vbox_showcase.add<Box>(Constraint::percent(50))
                .border_style(BorderStyle::Rounded)
                .fg(Color::Cyan)
                .title(" Vbox Top ");
            vbox_showcase.add<Box>(Constraint::fill())
                .border_style(BorderStyle::Single)
                .fg(Color::Emerald)
                .title(" Vbox Bottom ");
            vbox_showcase.draw(frame, vbox_area);

            auto [vb_t, vb_b] = Layout::vertical(
                vbox_area,
                {Constraint::percent(50), Constraint::fill()}
            );
            Text("Vbox Child 1 (50%)\nAuto flex stacked").draw(frame, vb_t.inset(1, 1, 1, 1));
            Text("Vbox Child 2 (fill)\nNested composite").draw(frame, vb_b.inset(1, 1, 1, 1));

            // 3. Background Fill Demonstration (Widget 4: Fill) with animated breathing color
            Box()
                .title(" Layered Composite Containers & Animated Background Fill (Widget: Fill) ")
                .border_style(BorderStyle::Rounded)
                .draw(frame, canvas_row);

            Area inner_canvas = canvas_row.inset(1, 2, 1, 2);

            // Animated breathing background color
            uint8_t bg_b = static_cast<uint8_t>(25 + static_cast<int>(12.0 * std::sin(anim_time * 2.0)));
            uint32_t dynamic_bg = Color::rgb(18, 22, bg_b);

            static constexpr std::array<char32_t, 5> fill_glyphs = {U'·', U'•', U'~', U'◦', U'✦'};
            Fill(dynamic_bg)
                .char_code(fill_glyphs[fill_glyph_idx % fill_glyphs.size()])
                .fg(Color::rgb(60, 75, 95))
                .draw(frame, inner_canvas);

            // Layered floating card on top of the fill with glowing shifting border
            Area floating_box = Layout::center(inner_canvas, 52, 6);
            uint8_t glow_r = static_cast<uint8_t>(127.5 * (1.0 + std::sin(anim_time * 2.0)));
            uint8_t glow_g = static_cast<uint8_t>(127.5 * (1.0 + std::sin(anim_time * 2.0 + 2.094)));
            uint8_t glow_b = static_cast<uint8_t>(127.5 * (1.0 + std::sin(anim_time * 2.0 + 4.188)));
            uint32_t border_glow = Color::rgb(glow_r, glow_g, glow_b);

            Box()
                .border_style(BorderStyle::Rounded)
                .fg(border_glow)
                .bg(Color::rgb(15, 20, 28))
                .title(" Floating Card with TrueColor Glow ")
                .draw(frame, floating_box);

            auto [card_msg, card_btn_area] = Layout::vertical(
                floating_box.inset(1, 2, 1, 2),
                {Constraint::fill(), Constraint::fixed(1)}
            );

            Text("This floating panel rests atop a dynamic Fill canvas.\nkoto diffs every cell to render tear-free at 60 FPS.")
                .align(TextAlign::Center)
                .draw(frame, card_msg);

            Button(" Cycle Fill Glyph ")
                .id(Id("btn_cycle_fill"))
                .focus(fm)
                .focused_border_fg(Color::Cyan)
                .on_click([&]() {
                    fill_glyph_idx = (fill_glyph_idx + 1) % fill_glyphs.size();
                    status_msg = "Changed background fill glyph";
                })
                .draw(frame, card_btn_area);
        }
        else if (cur_tab == 3)
        {
            // -----------------------------------------------------------------
            // TAB 4: Typography & Colors (Text styling, colors, rainbow wave, marquee)
            // -----------------------------------------------------------------
            auto [rainbow_row, palette_row, text_row, marquee_row] = Layout::vertical(
                body_area,
                {Constraint::fixed(3), Constraint::fixed(4), Constraint::fill(), Constraint::fixed(3)}
            );

            // 1. Continuous TrueColor Rainbow Wave (60 FPS animated spectrum)
            Box()
                .title(" 24-bit TrueColor Continuous Wave [60 FPS Pure Math] ")
                .border_style(BorderStyle::Rounded)
                .draw(frame, rainbow_row);

            Area wave_inner = rainbow_row.inset(1, 2, 1, 2);
            for (int c = 0; c < wave_inner.width; ++c)
            {
                double phase = anim_time * 3.5 + static_cast<double>(c) * 0.15;
                uint8_t r = static_cast<uint8_t>(127.5 * (1.0 + std::sin(phase)));
                uint8_t g = static_cast<uint8_t>(127.5 * (1.0 + std::sin(phase + 2.094395)));
                uint8_t b = static_cast<uint8_t>(127.5 * (1.0 + std::sin(phase + 4.188790)));
                uint32_t col = Color::rgb(r, g, b);
                for (int r_idx = 0; r_idx < wave_inner.height; ++r_idx)
                {
                    frame.set_cell(wave_inner.row + r_idx, wave_inner.col + c, Cell{U'█', col, Color::rgb(15, 18, 25)});
                }
            }

            // 2. Color Palette Chips
            Box()
                .title(" Primary Color Swatches ")
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

            // 3. Typography Attributes & Word Wrapping
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
            Text(" Inverted reverse badge ").reverse().fg(invert_palette ? Color::Amber : Color::Emerald).draw(frame, style_items[5]);

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

            // 4. Smooth Scrolling Marquee News Ticker
            Box()
                .title(" Live News Ticker (Scrolling Marquee) ")
                .border_style(BorderStyle::Single)
                .fg(Color::DarkGray)
                .draw(frame, marquee_row);

            static constexpr std::string_view marquee_text =
                " ★ koto v1.0.0 ★ Pure C++23 Immediate-Mode TUI ★ 14 Native Widgets ★ Zero Heap Allocations in Render Loop ★ 60 FPS Tear-Free Diffing ★ Full Unicode Grapheme Cluster Engine ★ ";

            Area mq_inner = marquee_row.inset(1, 2, 1, 2);
            if (mq_inner.is_valid() && mq_inner.width > 0)
            {
                std::string visible_marquee;
                visible_marquee.reserve(static_cast<size_t>(mq_inner.width));
                size_t text_len = marquee_text.size();
                for (int i = 0; i < mq_inner.width; ++i)
                {
                    size_t idx = (marquee_offset + static_cast<size_t>(i)) % text_len;
                    visible_marquee.push_back(marquee_text[idx]);
                }
                Text(visible_marquee)
                    .fg(Color::Cyan)
                    .bold()
                    .draw(frame, mq_inner);
            }
        }

        // 4. Modal Dialog Overlay (Widget 11: Clear, Widget 1: Box, Widget 2: Button)
        if (show_modal)
        {
            Area modal_box = Layout::center(screen, 54, 11);
            if (modal_box.is_valid())
            {
                // Drop shadow
                Area shadow = modal_box.inset(0, 0, -1, -2);
                Fill(Color::rgb(10, 10, 15)).char_code(U'▒').draw(frame, shadow);

                // Erase background cells cleanly (Widget 3: Clear)
                Clear().draw(frame, modal_box);

                // Pulsing modal border color
                uint8_t m_r = static_cast<uint8_t>(220 + static_cast<int>(35.0 * std::sin(anim_time * 4.0)));
                uint32_t modal_border_col = Color::rgb(m_r, 160, 40);

                Box()
                    .title(" Modal Focus Trap Demo ")
                    .title_align(TitleAlign::Center)
                    .border_style(BorderStyle::Double)
                    .fg(modal_border_col)
                    .bg(Color::rgb(15, 20, 30))
                    .draw(frame, modal_box);

                Area inner_modal = modal_box.inset(1, 3, 1, 3);
                auto [m_msg, m_actions] = Layout::vertical(
                    inner_modal,
                    {Constraint::fill(), Constraint::fixed(3)}
                );

                Text("Focus is strictly confined to this modal dialog!\n"
                     "Background widgets are shielded from mouse & keys.\n"
                     "Click either button with the Mouse, or press [Esc].")
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

        // 5. Global Footer / Status Bar with Live Mouse Coordinates & Hit Target
        Box()
            .border_style(BorderStyle::Single)
            .fg(Color::DarkGray)
            .draw(frame, footer_area);

        auto [status_col, mouse_col, hints_col] = Layout::horizontal(
            footer_area.inset(0, 1, 0, 1),
            {Constraint::percent(35), Constraint::percent(35), Constraint::fill()}
        );

        Text(" " + status_msg)
            .fg(Color::Emerald)
            .draw(frame, status_col);

        Text(mouse_status)
            .fg(Color::Cyan)
            .draw(frame, mouse_col);

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
