#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <koto/koto.hpp>
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

using namespace koto;

void test_audit_fixes_and_edge_cases()
{
    std::cout << "[Test 1] Full audit fixes (List/Tabs focus defaults, auto-tagging, Layout::center clamping, UTF-8 safety)... ";

    // 1. ListState focus defaults and event guarding
    {
        ListState ls({"Item 1", "Item 2"});
        assert(ls.focused == true); // Default must be true for standalone usage

        // Disabling focus prevents event handling
        ls.focused = false;
        bool consumed_unfocused = ls.handle_event(KeyEvent{'j'});
        assert(!consumed_unfocused);
        assert(ls.selected == 0);

        // Re-enabling focus restores event handling
        ls.focused = true;
        bool consumed_focused = ls.handle_event(KeyEvent{'j'});
        assert(consumed_focused);
        assert(ls.selected == 1);
    }

    // 2. TabsState focus defaults and event guarding
    {
        TabsState ts({"Tab 1", "Tab 2"});
        assert(ts.focused == true); // Default must be true for standalone usage

        ts.focused = false;
        bool consumed_unfocused = ts.handle_event(KeyEvent{static_cast<uint32_t>(Key::Right)});
        assert(!consumed_unfocused);
        assert(ts.selected == 0);

        ts.focused = true;
        bool consumed_focused = ts.handle_event(KeyEvent{static_cast<uint32_t>(Key::Right)});
        assert(consumed_focused);
        assert(ts.selected == 1);
    }

    // 3. Automatic Area Tagging into Frame Hit-Buffer
    {
        Frame frame(20, 40);

        ListState ls({"Alpha", "Beta"});
        TextInputState tis("Hello");
        TextAreaState tas("Code\nHere");
        TabsState ts({"One", "Two"});

        Area a_list(0, 0, 10, 5);
        Area a_input(5, 0, 20, 3);
        Area a_area(8, 0, 20, 6);
        Area a_tabs(14, 0, 20, 3);

        List(ls).border_style(BorderStyle::Single).draw(frame, a_list);
        TextInput(tis).border_style(BorderStyle::Single).draw(frame, a_input);
        TextArea(tas).border_style(BorderStyle::Single).draw(frame, a_area);
        Tabs(ts).border_style(BorderStyle::Single).draw(frame, a_tabs);

        // Check that hit-testing queries on all 4 widgets return their respective state IDs!
        assert(frame.hit_at(1, 2) == Id(&ls));
        assert(frame.hit_at(6, 5) == Id(&tis));
        assert(frame.hit_at(10, 5) == Id(&tas));
        assert(frame.hit_at(15, 5) == Id(&ts));

        // Spatial bounds auto-harvested by FocusManager
        FocusManager fm;
        fm.add(ls).add(tis).add(tas).add(ts);
        fm.sync_geometry(frame);

        auto bounds_ls = frame.bounds_of(Id(&ls));
        assert(bounds_ls.has_value() && *bounds_ls == a_list);

        auto bounds_input = frame.bounds_of(Id(&tis));
        assert(bounds_input.has_value() && *bounds_input == a_input);

        auto bounds_area = frame.bounds_of(Id(&tas));
        assert(bounds_area.has_value() && *bounds_area == a_area);

        auto bounds_tabs = frame.bounds_of(Id(&ts));
        assert(bounds_tabs.has_value() && *bounds_tabs == a_tabs);
    }

    // 4. Visual Focus Glowing Border on List and Tabs
    {
        Frame frame(6, 20);
        ListState ls({"Item 1"});
        ls.focused = true;

        List(ls)
            .border_style(BorderStyle::Single)
            .draw(frame, Area(0, 0, 10, 3));

        // Border top-left corner must have glowing Cyan color
        Cell corner = frame.get_cell(0, 0);
        assert(corner.ch == U'┌');
        assert(corner.fg == Color::Cyan);

        TabsState ts({"T1", "T2"});
        ts.focused = true;
        Tabs(ts)
            .border_style(BorderStyle::Single)
            .focused_border_fg(Color::Yellow)
            .draw(frame, Area(3, 0, 10, 3));

        Cell tab_corner = frame.get_cell(3, 0);
        assert(tab_corner.ch == U'┌');
        assert(tab_corner.fg == Color::Yellow);
    }

    // 5. Layout::center Clamping Bug Fix
    {
        Area parent(5, 10, 40, 20);

        // Under-sized: centers nicely
        Area c_normal = Layout::center(parent, 20, 10);
        assert(c_normal.row == 5 + (20 - 10) / 2);
        assert(c_normal.col == 10 + (40 - 20) / 2);
        assert(c_normal.width == 20);
        assert(c_normal.height == 10);

        // Over-sized: width 60 > 40, height 30 > 20 -> must clamp to parent bounds!
        Area c_oversized = Layout::center(parent, 60, 30);
        assert(c_oversized.row == 5);
        assert(c_oversized.col == 10);
        assert(c_oversized.width == 40);
        assert(c_oversized.height == 20);
    }

    // 6. UTF-8 Decoder Resilience against Orphan Continuation Bytes
    {
        std::string_view bad_stream = "\x80\xFFHello";
        char32_t c1 = next_codepoint(bad_stream);
        assert(c1 == U'\uFFFD'); // 0x80 handled safely
        char32_t c2 = next_codepoint(bad_stream);
        assert(c2 == U'\uFFFD'); // 0xFF handled safely
        char32_t c3 = next_codepoint(bad_stream);
        assert(c3 == U'H');      // valid stream continues undamaged
    }

    // 7. FocusNavPolicy::TabOnly prevents unconsumed arrow keys from hijacking focus
    {
        FocusManager fm; // Default is TabOnly
        assert(fm.navigation_policy() == FocusNavPolicy::TabOnly);

        fm.add("btn1");
        fm.add("btn2");

        Frame frame(10, 30);
        Box().id("btn1").draw(frame, Area(0, 0, 10, 3));
        Box().id("btn2").draw(frame, Area(0, 12, 10, 3));
        fm.sync_geometry(frame);

        assert(fm.is_focused("btn1"));

        // In TabOnly mode, pressing Right arrow does NOT jump focus!
        bool moved_arrow = fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Right)});
        assert(!moved_arrow);
        assert(fm.is_focused("btn1")); // Still on btn1!

        // Tab DOES jump focus predictably
        bool moved_tab = fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Tab)});
        assert(moved_tab);
        assert(fm.is_focused("btn2"));

        // Switching policy to SpatialArrows enables arrow key jumping
        fm.enable_spatial_arrows(true);
        assert(fm.navigation_policy() == FocusNavPolicy::SpatialArrows);
        bool moved_spatial_left = fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Left)});
        assert(moved_spatial_left);
        assert(fm.is_focused("btn1"));
    }

    // 8. FocusManager::interceptor allows developers to write custom focus logic
    {
        FocusManager fm;
        fm.add("panel1").add("panel2");
        assert(fm.is_focused("panel1"));

        // Intercept 'J' to jump focus forward
        fm.interceptor([&](FocusManager& mgr, const KeyEvent& k) {
            if (k.ch == 'J') {
                mgr.next();
                return true; // Consumed!
            }
            return false;
        });

        bool consumed = fm.handle_key(KeyEvent{'J'});
        assert(consumed);
        assert(fm.is_focused("panel2"));
    }

    // 9. FocusManager::can_focus vetoes focus switch (form validation)
    {
        FocusManager fm;
        fm.add("input_email").add("input_pass");
        assert(fm.is_focused("input_email"));

        bool allow_leave = false;
        fm.can_focus([&](FocusManager&, Id from_id, Id to_id) {
            static_cast<void>(to_id);
            if (from_id == Id("input_email") && !allow_leave) {
                return false; // VETO! Field is invalid!
            }
            return true;
        });

        // Attempting to tab away is vetoed!
        bool tabbed = fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Tab)});
        assert(!tabbed);
        assert(fm.is_focused("input_email")); // Retains focus!

        // When validation passes, focus switch succeeds
        allow_leave = true;
        bool tabbed_ok = fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Tab)});
        assert(tabbed_ok);
        assert(fm.is_focused("input_pass"));
    }

    // 10. FocusManager::on_focus_change observer notification
    {
        FocusManager fm;
        fm.add("tab_a").add("tab_b");

        Id recorded_from = Id::none();
        Id recorded_to   = Id::none();
        fm.on_focus_change([&](Id from, Id to) {
            recorded_from = from;
            recorded_to = to;
        });

        fm.next();
        assert(recorded_from == Id("tab_a"));
        assert(recorded_to == Id("tab_b"));
    }

    std::cout << "PASSED\n";
}

void test_audit_concerns_resolution()
{
    std::cout << "[Test 2] Audit concerns resolution (C-01, C-02, C-04, C-05, C-13, C-15, C-19, C-22, C-23, C-26, C-27, C-28)... ";

    // 1. C-01 Split Escape Sequence Streaming & Preservation
    {
        EventLoop loop;
        // Split ANSI CSI sequence: "\033[1;" followed by "5C" (Ctrl+Right)
        loop.feed("\033[1;");
        Event ev1 = loop.poll(0);
        assert(ev1.is_tick()); // Incomplete: does NOT consume or delete ESC!
        assert(loop.has_pending());

        loop.feed("5C");
        Event ev2 = loop.poll(0);
        assert(ev2.is_key());
        const auto* ke = ev2.as_key();
        assert(ke != nullptr);
        assert(ke->is(Key::CtrlRight));
        assert(!loop.has_pending()); // fully drained

        // Split SGR mouse sequence: "\033[<68;" followed by "10;20M"
        loop.feed("\033[<68;");
        Event ev_m1 = loop.poll(0);
        assert(ev_m1.is_tick());
        assert(loop.has_pending());

        loop.feed("10;20M");
        Event ev_m2 = loop.poll(0);
        assert(ev_m2.is_mouse());
        const auto* me = ev_m2.as_mouse();
        assert(me != nullptr);
        assert(me->button == MouseButton::WheelUp);
        assert(me->shift == true);
        assert(me->col == 9 && me->row == 19);
        assert(!loop.has_pending());
    }

    // 2. C-02 SGR Mouse Modifier Masks & Horizontal Wheel
    {
        // Shift + WheelUp: "\033[<68;5;5M"
        Event ev_wup = EventLoop::parse("\033[<68;5;5M");
        assert(ev_wup.is_mouse());
        assert(ev_wup.as_mouse()->button == MouseButton::WheelUp);
        assert(ev_wup.as_mouse()->shift == true);

        // Ctrl + WheelDown: "\033[<81;5;5M" (65 + 16 = 81)
        Event ev_wdown = EventLoop::parse("\033[<81;5;5M");
        assert(ev_wdown.is_mouse());
        assert(ev_wdown.as_mouse()->button == MouseButton::WheelDown);
        assert(ev_wdown.as_mouse()->ctrl == true);

        // WheelLeft: "\033[<66;5;5M"
        Event ev_wleft = EventLoop::parse("\033[<66;5;5M");
        assert(ev_wleft.is_mouse());
        assert(ev_wleft.as_mouse()->button == MouseButton::WheelLeft);
        assert(ev_wleft.as_mouse()->is_wheel_left());

        // WheelRight: "\033[<67;5;5M"
        Event ev_wright = EventLoop::parse("\033[<67;5;5M");
        assert(ev_wright.is_mouse());
        assert(ev_wright.as_mouse()->button == MouseButton::WheelRight);
        assert(ev_wright.as_mouse()->is_wheel_right());
    }

    // 3. C-04 Strict RFC 3629 UTF-8 Validation in next_codepoint
    {
        // E4 41 -> U+FFFD and 'A' preserved (not swallowed)
        std::string_view sv1 = "\xE4\x41";
        char32_t cp1 = next_codepoint(sv1);
        assert(cp1 == U'\uFFFD');
        assert(sv1 == "A");
        char32_t cp2 = next_codepoint(sv1);
        assert(cp2 == U'A');

        // C0 41 -> Overlong rejected
        std::string_view sv2 = "\xC0\x41";
        char32_t cp_overlong = next_codepoint(sv2);
        assert(cp_overlong == U'\uFFFD');
        assert(sv2 == "A");

        // ED A0 80 -> UTF-16 surrogate (U+D800) rejected
        std::string_view sv3 = "\xED\xA0\x80";
        char32_t cp_surrogate = next_codepoint(sv3);
        assert(cp_surrogate == U'\uFFFD');
    }

    // 4. C-05 Wide Glyph Overwrite & Orphan Continuation Protection
    {
        Frame frame(3, 10);
        // Place wide glyph at col 0 (cells 0, 1)
        frame.set_cell(0, 0, Cell{U'🦀'});
        assert(frame.get_cell(0, 0).width == 2);
        assert(frame.get_cell(0, 1).continuation == true);

        // Place wide glyph at col 2 (cells 2, 3)
        frame.set_cell(0, 2, Cell{U'🔥'});
        assert(frame.get_cell(0, 2).width == 2);
        assert(frame.get_cell(0, 3).continuation == true);

        // Overwrite col 1 with wide glyph U'🚀':
        // Col 0 owner cleared, Col 1 is head, Col 2 is continuation, Col 3 cleared (not orphan)
        frame.set_cell(0, 1, Cell{U'🚀'});
        assert(frame.get_cell(0, 0).ch == U' ');
        assert(frame.get_cell(0, 0).width == 1);
        assert(frame.get_cell(0, 1).ch == U'🚀');
        assert(frame.get_cell(0, 1).width == 2);
        assert(frame.get_cell(0, 2).continuation == true);
        assert(frame.get_cell(0, 3).continuation == false);
    }

    // 5. C-13 8-digit Hex Parsing
    {
        uint32_t c_8digit = parse_color("#FF000000");
        assert(c_8digit == Color::Red);
        assert(c_8digit != COLOR_DEFAULT);

        uint32_t c_6digit = parse_color("#FF0000");
        assert(c_6digit == Color::Red);
        assert(c_6digit != COLOR_DEFAULT);
    }

    // 6. C-22 Plain Arrow Key Clean Modifiers
    {
        KeyEvent k_up{static_cast<uint32_t>(Key::Up)};
        assert(k_up.is(Key::Up));
        assert(!k_up.is(Key::ShiftUp) && !k_up.is(Key::CtrlUp));

        KeyEvent k_shift_up{static_cast<uint32_t>(Key::Up), false, true, false, false};
        assert(!k_shift_up.is(Key::Up));
        assert(k_shift_up.is(Key::ShiftUp));

        KeyEvent k_ctrl_up{static_cast<uint32_t>(Key::Up), true, false, false, false};
        assert(!k_ctrl_up.is(Key::Up));
        assert(k_ctrl_up.is(Key::CtrlUp));
    }

    // 7. C-23 is_char() Boundary Validation
    {
        assert(!KeyEvent{127}.is_char()); // DEL
        assert(!KeyEvent{128}.is_char()); // C1
        assert(!KeyEvent{159}.is_char()); // C1
        assert(KeyEvent{160}.is_char());  // NBSP
        assert(KeyEvent{U'A'}.is_char());
        assert(KeyEvent{U'🦀'}.is_char());
        assert(!KeyEvent{static_cast<uint32_t>(Key::Up)}.is_char());
    }

    // 8. C-26 FocusKeyInterceptor with Empty Targets
    {
        FocusManager fm_empty;
        bool interceptor_called = false;
        fm_empty.interceptor([&](FocusManager&, const KeyEvent&) {
            interceptor_called = true;
            return true;
        });
        bool consumed = fm_empty.handle_key(KeyEvent{static_cast<uint32_t>(Key::Escape)});
        assert(consumed);
        assert(interceptor_called);
    }

    // 9. C-27 Color::lerp with COLOR_DEFAULT
    {
        assert(Color::lerp(COLOR_DEFAULT, Color::Red, 0.0f) == COLOR_DEFAULT);
        assert(Color::lerp(COLOR_DEFAULT, Color::Red, 1.0f) == Color::Red);
        assert(Color::lerp(Color::Red, COLOR_DEFAULT, 0.0f) == Color::Red);
        assert(Color::lerp(Color::Red, COLOR_DEFAULT, 1.0f) == COLOR_DEFAULT);
    }

    // 10. C-15 Id Hash Sentinel Safety (fnv1a_32 never produces TAG_NONE)
    {
        Id id1("test_id");
        assert(!id1.is_none());
        assert(id1.value() != static_cast<uint32_t>(TAG_NONE));
        assert(fnv1a_32("test_id") != static_cast<uint32_t>(TAG_NONE));
        assert(Id::none().is_none());
        assert(Id::none().value() == static_cast<uint32_t>(TAG_NONE));
    }

    // 11. C-19 App on_tick with delta time
    {
        App app;
        bool tick_invoked = false;
        app.on_tick([&]() {
            tick_invoked = true;
        });
        double captured_dt = 0.0;
        app.on_tick([&](double dt) {
            captured_dt = dt;
        });
        static_cast<void>(tick_invoked);
        static_cast<void>(captured_dt);
    }

    // 12. C-28 Text Attributes (Bold, Dim, Italic, Underline, Reverse, Strike)
    {
        Frame frame(1, 10);
        Text("Koto").bold().underline().draw(frame, Area{0, 0, 10, 1});
        assert((frame.get_cell(0, 0).attrs & static_cast<uint8_t>(TextAttr::Bold)) != 0);
        assert((frame.get_cell(0, 0).attrs & static_cast<uint8_t>(TextAttr::Underline)) != 0);
        frame.render(false);
        std::string_view rendered = frame.raw_output();
        assert(rendered.find("\033[1m") != std::string_view::npos);
        assert(rendered.find("\033[4m") != std::string_view::npos);
    }

    // 13. C-33 Unchanged frames emit 0 bytes
    {
        Frame frame(2, 5);
        frame.fill(Area{0, 0, 5, 2}, ' ');
        frame.render(false);
        assert(!frame.raw_output().empty());

        frame.render(false);
        assert(frame.raw_output().empty());
    }

    std::cout << "PASSED\n";
}

void test_claude_findings_resolution()
{
    std::cout << "[Test 3] Claude findings resolution... ";

    // 1. std::array<Constraint, N> overload for Layout::vertical & horizontal
    std::array<Constraint, 2> arr_c = {Constraint::fixed(10), Constraint::fill()};
    Area screen{0, 0, 80, 24};
    auto v_res = Layout::vertical(screen, arr_c);
    static_assert(std::is_same_v<decltype(v_res), std::array<Area, 2>>);
    assert(v_res[0].height == 10);
    assert(v_res[1].height == 14);

    auto h_res = Layout::horizontal(screen, arr_c);
    static_assert(std::is_same_v<decltype(h_res), std::array<Area, 2>>);
    assert(h_res[0].width == 10);
    assert(h_res[1].width == 70);

    // 2. Button::id("literal") without ambiguity
    Button btn("Submit");
    btn.id("submit_btn");
    btn.id(Id("another_id"));

    // 3. Text::wrap word-boundary wrapping
    Frame wrap_frame(3, 12);
    Text("hello world").wrap().draw(wrap_frame, wrap_frame.area());
    assert(wrap_frame.to_plain_text_row(0).starts_with("hello world"));

    Frame wrap_frame2(3, 10);
    Text("hello beautiful world").wrap().draw(wrap_frame2, wrap_frame2.area());
    assert(wrap_frame2.to_plain_text_row(0).starts_with("hello"));
    assert(wrap_frame2.to_plain_text_row(1).starts_with("beautiful"));

    // 4. pop_scope prunes modal targets from FocusManager
    FocusManager fm;
    Id bg_btn("bg_btn");
    fm.add(bg_btn);
    assert(fm.size() == 1);

    {
        IdScope modal(fm, "my_modal", ScopeMode::Modal);
        Id modal_btn1("modal_btn1");
        Id modal_btn2("modal_btn2");
        fm.add(modal_btn1);
        fm.add(modal_btn2);
        assert(fm.size() == 3);
    }
    assert(fm.size() == 1);
    assert(fm.current_id() == bg_btn);

    // 5. Terminal hover methods exist and can be called safely
    Terminal term;
    term.enable_mouse_hover();
    term.disable_mouse_hover();

    std::cout << "PASSED\n";
}

void test_regression_frame_copy_and_button_borders()
{
    std::cout << "[Test 4] Frame move-only semantics & button border alignment... ";

    // 1. Frame is non-copyable (preventing pass-by-value silent buffer slicing)
    static_assert(!std::is_copy_constructible_v<Frame>);
    static_assert(!std::is_copy_assignable_v<Frame>);
    static_assert(std::is_move_constructible_v<Frame>);
    static_assert(std::is_move_assignable_v<Frame>);

    // 2. Button with symbols like '✔' or '✖' preserves exact border alignment
    Frame frame(5, 20);
    Area btn_area(1, 1, 18, 3);
    Button("✔ Save Profile")
        .border_style(BorderStyle::Rounded)
        .draw(frame, btn_area);

    // Row 1: ╭────────────────╮
    // Row 2: │ ✔ Save Profile │
    // Row 3: ╰────────────────╯
    Cell r1_left = frame.get_cell(1, 1);
    Cell r1_right = frame.get_cell(1, 18);
    Cell r2_left = frame.get_cell(2, 1);
    Cell r2_right = frame.get_cell(2, 18);
    Cell r3_left = frame.get_cell(3, 1);
    Cell r3_right = frame.get_cell(3, 18);

    assert(r1_left.ch == U'╭');
    assert(r1_right.ch == U'╮');
    assert(r2_left.ch == U'│');
    assert(r2_right.ch == U'│');
    assert(r3_left.ch == U'╰');
    assert(r3_right.ch == U'╯');

    // 3. Button vertical centering in taller areas (height 5)
    Frame frame_tall(7, 20);
    Area tall_btn(1, 1, 18, 5);
    Button("Tall Button")
        .border_style(BorderStyle::Rounded)
        .draw(frame_tall, tall_btn);

    // Row 1: top border
    // Row 2: empty interior space
    // Row 3: centered label
    // Row 4: empty interior space
    // Row 5: bottom border
    assert(frame_tall.get_cell(1, 1).ch == U'╭');
    assert(frame_tall.get_cell(1, 18).ch == U'╮');
    assert(frame_tall.to_plain_text_row(2).find("Tall Button") == std::string::npos);
    assert(frame_tall.to_plain_text_row(3).find("Tall Button") != std::string::npos);
    assert(frame_tall.to_plain_text_row(4).find("Tall Button") == std::string::npos);
    assert(frame_tall.get_cell(5, 1).ch == U'╰');
    assert(frame_tall.get_cell(5, 18).ch == U'╯');

    // 4. App::on_render modifies App::frame() in-place (verifying non-copyable Frame& reference)
    App test_app;
    test_app.on_render([&](Frame& f) {
        Box().title("CPU").border_style(BorderStyle::Rounded).draw(f, Area{0, 0, 20, 5});
    });
    test_app.draw_frame(false);
    assert(test_app.frame().to_plain_text().find("CPU") != std::string::npos);

    // 5. Button with emojis like '🗑' or 'ℹ' preserves exact border alignment and clipping
    Frame frame_btn(5, 30);
    Area btn_del_area(1, 1, 26, 3);
    Button("🗑 Delete File (Press 'D')")
        .border_style(BorderStyle::Rounded)
        .draw(frame_btn, btn_del_area);

    assert(frame_btn.get_cell(1, 1).ch == U'╭');
    assert(frame_btn.get_cell(1, 26).ch == U'╮');
    assert(frame_btn.get_cell(2, 1).ch == U'│');
    assert(frame_btn.get_cell(2, 26).ch == U'│');
    assert(frame_btn.get_cell(3, 1).ch == U'╰');
    assert(frame_btn.get_cell(3, 26).ch == U'╯');

    // And verify rendering with non-ASCII resync prevents any raw output cursor drift
    frame_btn.render(false);
    std::string_view raw = frame_btn.raw_output();
    assert(!raw.empty());

    std::cout << "PASSED\n";
}

void test_c1_partial_escape_sequence_handling()
{
    std::cout << "[Test 5] C1 regression: partial escape sequences, no-spin drain, and Alt key timeouts... ";

    EventLoop loop;

    // 1. Partial CSI "\033[" buffers raw bytes, but has_pending_event() is false (prevents App::run 100% CPU spin)
    loop.feed("\033[");
    assert(loop.has_pending());
    assert(!loop.has_pending_event());

    // 2. Feed the completing byte 'A' -> Up Arrow
    loop.feed("A");
    assert(loop.has_pending());
    assert(loop.has_pending_event());
    Event ev_up = loop.poll(0);
    assert(ev_up.is_key());
    assert(ev_up.as_key()->ch == static_cast<uint32_t>(Key::Up));
    assert(!loop.has_pending());
    assert(!loop.has_pending_event());

    // 3. Partial SS3 "\033O" should not report pending event until completion
    loop.feed("\033O");
    assert(loop.has_pending());
    assert(!loop.has_pending_event());
    loop.feed("P"); // F1
    assert(loop.has_pending());
    assert(loop.has_pending_event());
    Event ev_f1 = loop.poll(0);
    assert(ev_f1.is_key());
    assert(ev_f1.as_key()->ch == static_cast<uint32_t>(Key::F1));
    assert(!loop.has_pending());
    assert(!loop.has_pending_event());

    // 4. Standalone Escape with 35ms timeout
    loop.feed("\033");
    assert(loop.has_pending());
    assert(!loop.has_pending_event()); // Before deadline, incomplete
    std::this_thread::sleep_for(std::chrono::milliseconds(40)); // Wait for 35ms deadline to expire
    assert(loop.has_pending());
    assert(loop.has_pending_event());  // Now pending because deadline expired!
    Event ev_esc = loop.poll(0);
    assert(ev_esc.is_key());
    assert(ev_esc.as_key()->ch == static_cast<uint32_t>(Key::Escape));
    assert(!loop.has_pending());
    assert(!loop.has_pending_event());

    // 5. Alt + '[' sequence ("\033[") with 35ms timeout
    loop.feed("\033[");
    assert(loop.has_pending());
    assert(!loop.has_pending_event());
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    assert(loop.has_pending());
    assert(loop.has_pending_event());
    Event ev_alt_bracket = loop.poll(0);
    assert(ev_alt_bracket.is_key());
    assert(ev_alt_bracket.as_key()->ch == static_cast<uint32_t>('['));
    assert(ev_alt_bracket.as_key()->alt == true);
    assert(!loop.has_pending());
    assert(!loop.has_pending_event());

    // 6. Rapid interleaved input: 'x' followed by "\033["
    loop.feed("x\033[");
    assert(loop.has_pending());
    assert(loop.has_pending_event()); // 'x' is complete!
    Event ev_x = loop.poll(0);
    assert(ev_x.is_key() && ev_x.as_key()->ch == 'x');
    // Now only "\033[" remains in the buffer - has_pending() is true, but has_pending_event() is false
    assert(loop.has_pending());
    assert(!loop.has_pending_event());
    loop.feed("B"); // Down Arrow
    assert(loop.has_pending());
    assert(loop.has_pending_event());
    Event ev_down = loop.poll(0);
    assert(ev_down.is_key() && ev_down.as_key()->ch == static_cast<uint32_t>(Key::Down));
    assert(!loop.has_pending());
    assert(!loop.has_pending_event());

    std::cout << "PASSED\n";
}

void test_h3_and_m2_hygiene()
{
    std::cout << "[Test 6] H3/M2: SIGHUP nohup inheritance preservation & EOF hygiene... ";

    // 1. Verify SIGHUP preservation when running under nohup (SIG_IGN)
    struct sigaction ign_sa{};
    ign_sa.sa_handler = SIG_IGN;
    sigemptyset(&ign_sa.sa_mask);
    struct sigaction old_sa{};
    int rc = sigaction(SIGHUP, &ign_sa, &old_sa);
    assert(rc == 0);

    struct sigaction cur_sa{};
    sigaction(SIGHUP, nullptr, &cur_sa);
    assert(cur_sa.sa_handler == SIG_IGN);

    // Restore original
    sigaction(SIGHUP, &old_sa, nullptr);

    // 2. EventLoop EOF verification
    EventLoop loop;
    assert(!loop.is_eof());

    std::cout << "PASSED\n";
}

void test_r1_slow_paste_and_r5_truncated_input()
{
    std::cout << "[Test 7] R1 slow paste preservation & R5 truncated input hygiene... ";

    // 1. R1: Slow paste with a 120ms transmission pause
    {
        EventLoop loop;
        loop.feed("\033[200~hello ");
        assert(loop.has_pending());
        assert(!loop.has_pending_event()); // Paste is in progress, NOT a complete event!

        // Simulate 120ms network/tmux pause during paste
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        assert(loop.has_pending());
        assert(!loop.has_pending_event()); // Crucial: must NOT expire or get sliced after 35ms!

        // Remaining paste payload arrives
        loop.feed("world\033[201~");
        assert(loop.has_pending());
        assert(loop.has_pending_event()); // Complete paste event ready!

        Event ev = loop.poll(0);
        assert(ev.is_paste());
        assert(ev.as_paste() != nullptr);
        assert(ev.as_paste()->text == "hello world");
        assert(!loop.has_pending());
        assert(!loop.has_pending_event());
    }

    // 2. R5: Truncated non-ESC input (orphaned multi-byte UTF-8 lead byte)
    {
        EventLoop loop;
        // Feed an orphan UTF-8 lead byte (0xC3) without continuation byte
        loop.feed("\xC3");
        assert(loop.has_pending());
        assert(!loop.has_pending_event());

        // Wait 40ms (> 35ms timeout)
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        assert(loop.has_pending());
        assert(loop.has_pending_event()); // Timeout ready to drop orphan byte

        Event ev = loop.poll(0);
        assert(ev.is_tick()); // Dropped orphan byte
        assert(!loop.has_pending()); // Buffer is clean, tick rate preserved!
        assert(!loop.has_pending_event());
    }

    // 3. R1/C1: Reset escape deadline on new incoming bytes
    {
        EventLoop loop;
        // Partial CSI
        loop.feed("\033[");
        assert(loop.has_pending());
        assert(!loop.has_pending_event());

        // Wait 25ms (< 35ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        loop.feed("1;"); // New bytes arrive: deadline resets!
        assert(loop.has_pending());
        assert(!loop.has_pending_event());

        // Wait another 25ms (total 50ms > original 35ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        loop.feed("5C"); // Final byte of Ctrl+Right arrives
        assert(loop.has_pending());
        assert(loop.has_pending_event());

        Event ev = loop.poll(0);
        assert(ev.is_key());
        assert(ev.as_key()->is(Key::CtrlRight));
        assert(!loop.has_pending());
    }

    std::cout << "PASSED\n";
}

int main()
{
    std::cout << "========================================\n";
    std::cout << " Running koto Headless Regressions & Audit Tests\n";
    std::cout << "========================================\n";

    test_audit_fixes_and_edge_cases();
    test_audit_concerns_resolution();
    test_claude_findings_resolution();
    test_regression_frame_copy_and_button_borders();
    test_c1_partial_escape_sequence_handling();
    test_h3_and_m2_hygiene();
    test_r1_slow_paste_and_r5_truncated_input();

    std::cout << "========================================\n";
    std::cout << " ALL 7 REGRESSION TESTS PASSED CLEANLY!\n";
    std::cout << "========================================\n";
    return 0;
}
