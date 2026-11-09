#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <koto/koto.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace koto;

void test_kitty_keyboard_and_paste()
{
    std::cout << "[Test 1] Kitty keyboard protocol (CSI u) & Bracketed paste mode... ";

    // 1. Kitty CSI u Disambiguated Enter
    {
        Event ev1 = EventLoop::parse("\033[13u");
        assert(ev1.is_key());
        assert(ev1.key().is(Key::Enter));
        assert(!ev1.key().ctrl && !ev1.key().shift && !ev1.key().alt && !ev1.key().meta);

        Event ev2 = EventLoop::parse("\033[13;2u"); // Shift + Enter
        assert(ev2.is_key());
        assert(ev2.key().is(Key::Enter));
        assert(ev2.key().shift);

        Event ev3 = EventLoop::parse("\033[13;5u"); // Ctrl + Enter
        assert(ev3.is_key());
        assert(ev3.key().is(Key::Enter));
        assert(ev3.key().ctrl);

        Event ev4 = EventLoop::parse("\033[13;5:1u"); // Ctrl + Enter (press event)
        assert(ev4.is_key());
        assert(ev4.key().is(Key::Enter));
        assert(ev4.key().ctrl);

        Event ev5 = EventLoop::parse("\033[13;5:3u"); // Release event -> ignored
        assert(ev5.is_tick());
    }

    // 2. Kitty CSI u Disambiguated Tab
    {
        Event ev1 = EventLoop::parse("\033[9u");
        assert(ev1.is_key());
        assert(ev1.key().is(Key::Tab));

        Event ev2 = EventLoop::parse("\033[9;2u"); // Shift + Tab
        assert(ev2.is_key());
        assert(ev2.key().is(Key::Tab));
        assert(ev2.key().is(Key::BackTab));
        assert(ev2.key().shift);

        Event ev3 = EventLoop::parse("\033[9;5u"); // Ctrl + Tab
        assert(ev3.is_key());
        assert(ev3.key().is(Key::Tab));
        assert(ev3.key().ctrl);
    }

    // 3. Kitty CSI u Disambiguated Escape & Backspace
    {
        Event ev1 = EventLoop::parse("\033[27u");
        assert(ev1.is_key());
        assert(ev1.key().is(Key::Escape));

        Event ev2 = EventLoop::parse("\033[27;3u"); // Alt + Escape
        assert(ev2.is_key());
        assert(ev2.key().is(Key::Escape));
        assert(ev2.key().alt);

        Event ev3 = EventLoop::parse("\033[127u");
        assert(ev3.is_key());
        assert(ev3.key().is(Key::Backspace));

        Event ev4 = EventLoop::parse("\033[127;5u"); // Ctrl + Backspace
        assert(ev4.is_key());
        assert(ev4.key().is(Key::Backspace));
        assert(ev4.key().ctrl);
    }

    // 4. Meta / Super modifier support (bitmask 8 / mod param 9)
    {
        Event ev1 = EventLoop::parse("\033[97;9u"); // Meta + 'a'
        assert(ev1.is_key());
        assert(ev1.key().ch == 'a');
        assert(ev1.key().meta);
        assert(!ev1.key().shift);

        Event ev2 = EventLoop::parse("\033[97;10u"); // Meta + Shift + 'a' -> 'A'
        assert(ev2.is_key());
        assert(ev2.key().ch == 'A');
        assert(ev2.key().meta);
        assert(ev2.key().shift);

        Event ev3 = EventLoop::parse("\033[1;9A"); // Meta + Up
        assert(ev3.is_key());
        assert(ev3.key().is(Key::Up));
        assert(ev3.key().meta);
    }

    // 5. Kitty Functional Keys (Private Use Area)
    {
        Event ev1 = EventLoop::parse("\033[57414u"); // KP_ENTER
        assert(ev1.is_key());
        assert(ev1.key().is(Key::Enter));

        Event ev2 = EventLoop::parse("\033[57419u"); // KP_UP
        assert(ev2.is_key());
        assert(ev2.key().is(Key::Up));

        Event ev3 = EventLoop::parse("\033[57426u"); // KP_DELETE
        assert(ev3.is_key());
        assert(ev3.key().is(Key::Delete));

        Event ev4 = EventLoop::parse("\033[57358u"); // CAPS_LOCK -> ignored
        assert(ev4.is_tick());
    }

    // 6. Bracketed Paste Mode extraction & CRLF normalization
    {
        Event ev1 = EventLoop::parse("\033[200~Hello World\033[201~");
        assert(ev1.is_paste());
        assert(ev1.paste().text == "Hello World");

        Event ev2 = EventLoop::parse("\033[200~Line 1\r\nLine 2\rLine 3\nLine 4\033[201~");
        assert(ev2.is_paste());
        assert(ev2.paste().text == "Line 1\nLine 2\nLine 3\nLine 4");
    }

    std::cout << "PASSED\n";
}

void test_hit_buffer_and_box_focus()
{
    std::cout << "[Test 2] Frame hit-buffer (-1 default tag) & Box visual focus... ";

    // 1. Initial hit buffer filled with TAG_NONE (-1)
    {
        Frame frame(4, 10);
        for (int r = 0; r < 4; ++r)
        {
            for (int c = 0; c < 10; ++c)
            {
                assert(frame.hit_at(r, c) == TAG_NONE);
            }
        }
        // Out of bounds queries return TAG_NONE (-1)
        assert(frame.hit_at(-1, 0) == TAG_NONE);
        assert(frame.hit_at(4, 10) == TAG_NONE);
    }

    // 2. set_cell with tag_id and tag_area
    {
        Frame frame(10, 20);
        frame.set_cell(2, 3, Cell{U'X', Color::White, Color::Black}, 101);
        assert(frame.hit_at(2, 3) == 101);
        assert(frame.hit_at(2, 4) == TAG_NONE);

        // tag_area
        Area dialog(4, 5, 8, 4); // rows 4..7, cols 5..12
        frame.tag_area(dialog, 202);
        assert(frame.hit_at(4, 5) == 202);
        assert(frame.hit_at(7, 12) == 202);
        assert(frame.hit_at(3, 5) == TAG_NONE);
        assert(frame.hit_at(8, 12) == TAG_NONE);

        // Overlapping draw (modal popup overwriting background area)
        Area modal(5, 6, 4, 2); // rows 5..6, cols 6..9
        frame.tag_area(modal, 999);
        assert(frame.hit_at(5, 6) == 999);
        assert(frame.hit_at(4, 5) == 202); // surrounding area remains 202!
    }

    // 3. Box with .id(int) and .focused(bool)
    {
        Frame frame(6, 20);
        Box()
            .title("Panel A")
            .id(1)
            .focused(false)
            .draw(frame, Area(0, 0, 10, 6));

        // Unfocused border should match m_fg (default)
        assert(frame.hit_at(0, 0) == 1);
        assert(frame.hit_at(2, 4) == 1); // interior tagged even when not filled!
        assert(frame.hit_at(0, 11) == TAG_NONE);

        // Focused box
        Box()
            .title("Panel B")
            .id(2)
            .focused(true)
            .focused_border_fg(Color::Emerald)
            .draw(frame, Area(0, 10, 10, 6));

        assert(frame.hit_at(0, 10) == 2);
        assert(frame.hit_at(3, 15) == 2);
        // Border cell color should be Color::Emerald
        assert(frame.get_cell(0, 10).fg == Color::Emerald);
    }

    std::cout << "PASSED\n";
}

enum class TestPanel { Sidebar, Editor, Output };

void test_universal_id_and_focus_manager()
{
    std::cout << "[Test 3] Universal Id token & FocusManager (Tab cycle, key routing, mouse click)... ";

    // 1. Id construction from string, pointer, enum, multi-part
    {
        Id id_str("sidebar");
        assert(!id_str.is_none());
        assert(id_str == Id("sidebar"));
        assert(id_str != Id("other"));

        int dummy = 42;
        Id id_ptr(&dummy);
        assert(!id_ptr.is_none());
        assert(id_ptr == Id(&dummy));

        Id id_enum(TestPanel::Editor);
        assert(!id_enum.is_none());
        assert(id_enum == Id(TestPanel::Editor));

        Id part0("item", 0);
        Id part1("item", 1);
        assert(part0 != part1);
        assert(part0 == Id("item", 0));

        assert(Id::none().is_none());
    }

    // 2. FocusManager with rich states (TextInputState, ListState)
    {
        FocusManager fm;
        TextInputState user_in("Alice");
        TextInputState pass_in("");
        ListState files({"doc.txt", "main.cpp"});
        TextAreaState editor("code");

        fm.add(user_in);
        fm.add(pass_in);
        fm.add(files);
        fm.add(editor);

        // First registered element gains initial focus automatically
        assert(user_in.focused == true);
        assert(pass_in.focused == false);
        assert(files.focused == false);
        assert(editor.focused == false);
        assert(fm.is_focused(user_in));
        assert(!fm.is_focused(pass_in));

        // Tab -> cycle to next (pass_in)
        bool handled_tab = fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Tab)});
        assert(handled_tab);
        assert(user_in.focused == false);
        assert(pass_in.focused == true);
        assert(files.focused == false);
        assert(editor.focused == false);
        assert(fm.is_focused(pass_in));

        // Tab -> cycle to next (files)
        fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Tab)});
        assert(user_in.focused == false);
        assert(pass_in.focused == false);
        assert(files.focused == true);
        assert(editor.focused == false);
        assert(fm.is_focused(files));

        // Tab -> cycle to next (editor)
        fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Tab)});
        assert(user_in.focused == false);
        assert(pass_in.focused == false);
        assert(files.focused == false);
        assert(editor.focused == true);
        assert(fm.is_focused(editor));

        // BackTab -> cycle backwards (files)
        fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::BackTab)});
        assert(files.focused == true);
        assert(editor.focused == false);

        // BackTab -> cycle backwards (pass_in)
        fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::BackTab)});
        assert(pass_in.focused == true);
        assert(files.focused == false);

        // Key Routing: typing into focused pass_in
        KeyEvent key_k{'K', false, true, false, false}; // Shift+'k' -> 'K'
        bool routed = fm.handle_key(key_k);
        assert(routed);
        assert(pass_in.text() == "K");
        assert(user_in.text() == "Alice"); // unfocused element unmodified!
    }

    // 3. Stateless Box / Panel with custom key callback
    {
        FocusManager fm;
        bool logs_cleared = false;

        fm.add("logs", [&](const KeyEvent& k) {
            if (k.ch == 'c' || k.ch == 'C')
            {
                logs_cleared = true;
                return true;
            }
            return false;
        });

        assert(fm.is_focused("logs"));
        // 'c' triggers callback
        bool c_consumed = fm.handle_key(KeyEvent{'c'});
        assert(c_consumed);
        assert(logs_cleared);

        // 'x' not consumed by logs
        bool x_consumed = fm.handle_key(KeyEvent{'x'});
        assert(!x_consumed); // bubbles to app
    }

    // 4. Mouse click hit-testing with Frame
    {
        FocusManager fm;
        TextInputState input;
        fm.add(input, Id("my_input"));
        fm.add("card_box");

        // Initially my_input is focused
        assert(fm.is_focused("my_input"));
        assert(!fm.is_focused("card_box"));

        // Render card_box at (5, 5, 10, 4)
        Frame frame(15, 30);
        Box().id("card_box").draw(frame, Area(5, 5, 10, 4));

        // Click outside the box -> does not change focus
        bool clicked_outside = fm.handle_mouse_click(0, 0, frame);
        assert(!clicked_outside);
        assert(fm.is_focused("my_input"));

        // Click inside card_box at (6, 7) -> shifts focus to card_box!
        bool clicked_inside = fm.handle_mouse_click(6, 7, frame);
        assert(clicked_inside);
        assert(fm.is_focused("card_box"));
        assert(!fm.is_focused("my_input"));
        assert(input.focused == false);
    }

    // 5. Scoped IDs with RAII (avoiding collisions in reusable components)
    {
        FocusManager fm;
        Id id_p1;
        Id id_p2;
        {
            auto scope = fm.scope("player_1");
            id_p1 = fm.scoped_id("avatar");
            Id id_p1_again = fm.scoped_id("avatar");
            assert(id_p1 == id_p1_again);
        }
        {
            auto scope = fm.scope("player_2");
            id_p2 = fm.scoped_id("avatar");
        }
        // player_1::avatar != player_2::avatar
        assert(id_p1 != id_p2);
        // unscoped avatar differs from scoped avatar
        assert(id_p2 != fm.scoped_id("avatar"));
    }

    std::cout << "PASSED\n";
}

void test_spatial_navigation_and_modal_isolation()
{
    std::cout << "[Test 4] Spatial 2D navigation (auto-harvested bounds) & Modal focus trapping... ";

    // 1. Spatial 2D Navigation on a 3x3 Grid
    {
        Frame frame(15, 30);
        FocusManager fm;
        fm.enable_spatial_arrows(true);

        // Register 9 cells (pure logical IDs, zero coordinates passed!)
        for (size_t r = 0; r < 3; ++r)
        {
            for (size_t c = 0; c < 3; ++c)
            {
                fm.add(Id("cell", r, c));
            }
        }

        // Render pass: Area is dynamically calculated here!
        auto grid = Layout::grid_uniform(frame.area(), 3, 3);
        grid.for_each([&](size_t r, size_t c, const Area& cell_area) {
            Box().id(Id("cell", r, c)).draw(frame, cell_area);
        });

        // Frame auto-harvested all 9 bounding boxes!
        assert(frame.bounds_of(Id("cell", 0, 0)).has_value());
        assert(frame.bounds_of(Id("cell", 2, 2)).has_value());

        // Sync geometry from frame into focus manager
        fm.sync_geometry(frame);

        // Initially at (0, 0)
        assert(fm.is_focused(Id("cell", 0, 0)));

        // Move Right -> (0, 1)
        fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Right)});
        assert(fm.is_focused(Id("cell", 0, 1)));

        // Move Right -> (0, 2)
        fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Right)});
        assert(fm.is_focused(Id("cell", 0, 2)));

        // Move Right at right edge -> clamped (stays at 0, 2)
        bool clamped_right = fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Right)});
        assert(!clamped_right);
        assert(fm.is_focused(Id("cell", 0, 2)));

        // Move Down -> (1, 2)
        fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Down)});
        assert(fm.is_focused(Id("cell", 1, 2)));

        // Move Left -> (1, 1) [Center cell]
        fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Left)});
        assert(fm.is_focused(Id("cell", 1, 1)));

        // Move Up -> (0, 1)
        fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Up)});
        assert(fm.is_focused(Id("cell", 0, 1)));
    }

    // 2. Modal Focus Trapping & Background Shielding
    {
        Frame frame(20, 40);
        FocusManager fm;

        // Background UI: 2 panels with spatial arrow navigation enabled
        fm.enable_spatial_arrows(true);
        fm.add("sidebar");
        fm.add("main_editor");

        Box().id("sidebar").draw(frame, Area(0, 0, 10, 20));
        Box().id("main_editor").draw(frame, Area(0, 10, 30, 20));
        fm.sync_geometry(frame);

        // Move to main_editor
        fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Right)});
        assert(fm.is_focused("main_editor"));

        // Open Modal Dialog
        {
            auto modal = fm.modal_scope("dialog");

            fm.add("btn_cancel");
            fm.add("btn_confirm");

            // In modal dialog, cancel is on left, confirm is on right
            Box().id(fm.id("btn_cancel")).draw(frame, Area(10, 12, 8, 3));
            Box().id(fm.id("btn_confirm")).draw(frame, Area(10, 22, 8, 3));
            fm.sync_geometry(frame);

            // Focus automatically transferred to first modal button!
            assert(fm.is_focused("btn_cancel"));

            // Right arrow moves to btn_confirm
            fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Right)});
            assert(fm.is_focused("btn_confirm"));

            // Right arrow cannot escape modal to background editor!
            bool escaped_right = fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Right)});
            assert(!escaped_right);
            assert(fm.is_focused("btn_confirm"));

            // Up arrow cannot escape modal to background editor!
            bool escaped_up = fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Up)});
            assert(!escaped_up);
            assert(fm.is_focused("btn_confirm"));

            // Tab within modal wraps to btn_cancel, NOT background!
            fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Tab)});
            assert(fm.is_focused("btn_cancel"));

            // Mouse click on background sidebar at (2, 2) is shielded!
            bool click_handled = fm.handle_mouse_click(2, 2, frame);
            assert(click_handled); // Click absorbed!
            assert(fm.is_focused("btn_cancel")); // Still on modal!
            assert(!fm.is_focused("sidebar")); // Sidebar does NOT get focus!
        }
        // Modal scope ended (destructor ran and popped modal)

        // Focus automatically restored to main_editor!
        assert(fm.is_focused("main_editor"));
    }

    std::cout << "PASSED\n";
}

void test_app_use_focus_and_mouse()
{
    std::cout << "[Test 5] App::use_focus(fm) integration & SGR mouse parser... ";

    // 1. SGR Mouse Escape Sequence Parser
    {
        // Left click press at col 15, row 5 (1-based -> 0-based col 14, row 4)
        Event ev1 = EventLoop::parse("\033[<0;15;5M");
        assert(ev1.is_mouse());
        const auto* m1 = ev1.as_mouse();
        assert(m1 != nullptr);
        assert(m1->col == 14);
        assert(m1->row == 4);
        assert(m1->button == MouseButton::Left);
        assert(m1->action == MouseAction::Press);
        assert(!m1->shift && !m1->alt && !m1->ctrl);

        // Left click release at col 15, row 5
        Event ev2 = EventLoop::parse("\033[<0;15;5m");
        assert(ev2.is_mouse());
        const auto* m2 = ev2.as_mouse();
        assert(m2 != nullptr);
        assert(m2->col == 14);
        assert(m2->row == 4);
        assert(m2->action == MouseAction::Release);

        // Wheel up at col 20, row 8
        Event ev3 = EventLoop::parse("\033[<64;20;8M");
        assert(ev3.is_mouse());
        const auto* m3 = ev3.as_mouse();
        assert(m3 != nullptr);
        assert(m3->button == MouseButton::WheelUp);
        assert(m3->col == 19 && m3->row == 7);

        // Shift + Left Click at col 10, row 3
        Event ev4 = EventLoop::parse("\033[<4;10;3M");
        assert(ev4.is_mouse());
        const auto* m4 = ev4.as_mouse();
        assert(m4 != nullptr);
        assert(m4->button == MouseButton::Left);
        assert(m4->shift == true);
    }

    // 2. Full App::use_focus(fm) Lifecycle & Event Routing
    {
        App app;
        FocusManager fm;

        TextInputState user_in("Alice");
        TextInputState pass_in("");
        bool submit_clicked = false;

        fm.add(user_in).add(pass_in);
        fm.add("submit_btn", [&](const KeyEvent& k) {
            if (k.is(Key::Enter))
            {
                submit_clicked = true;
                return true;
            }
            return false;
        });

        // Plug focus manager into app
        app.use_focus(fm);

        // Setup UI layout
        app.on_render([&](Frame& frame) {
            Box().id(fm.id(user_in)).draw(frame, Area(0, 0, 20, 3));
            Box().id(fm.id(pass_in)).draw(frame, Area(4, 0, 20, 3));
            Box().id(fm.id("submit_btn")).draw(frame, Area(8, 0, 20, 3));
        });

        // Trigger render & geometry harvest (silent without flushing to terminal in test)
        app.draw_frame(/*flush=*/false);

        // 1. Initial state: user_in is focused
        assert(fm.is_focused(user_in));
        assert(!fm.is_focused(pass_in));

        // 2. Dispatch Tab key event through app
        app.dispatch_event(Event{KeyEvent{static_cast<uint32_t>(Key::Tab)}});
        assert(fm.is_focused(pass_in));
        assert(!fm.is_focused(user_in));

        // 3. Dispatch typing into pass_in
        app.dispatch_event(Event{KeyEvent{'P', false, true, false, false}});
        assert(pass_in.text() == "P");
        assert(user_in.text() == "Alice");

        // 4. Dispatch Mouse Click onto "submit_btn" Area (row 8..11, col 0..20)
        // Click at row 9, col 5
        app.dispatch_event(Event{MouseEvent{
            .row = 9,
            .col = 5,
            .button = MouseButton::Left,
            .action = MouseAction::Press,
            .shift = false,
            .alt = false,
            .ctrl = false
        }});
        assert(fm.is_focused("submit_btn"));
        assert(!fm.is_focused(pass_in));

        // 5. Press Enter on focused submit_btn -> triggers custom callback
        app.dispatch_event(Event{KeyEvent{static_cast<uint32_t>(Key::Enter)}});
        assert(submit_clicked);

        // 6. Unconsumed key bubbles to app.on_key
        bool quit_received = false;
        app.on_key([&](const KeyEvent& k) {
            if (k.ch == 'q') quit_received = true;
        });
        // 'q' is not consumed by submit_btn -> bubbles to app.on_key!
        app.dispatch_event(Event{KeyEvent{'q'}});
        assert(quit_received);
    }

    std::cout << "PASSED\n";
}

void test_subitem_mouse_interaction()
{
    std::cout << "[Test 6] Sub-item mouse click & wheel interactions (List, Tabs, FocusManager)... ";

    // 1. TabsState direct mouse click & wheel
    {
        Frame frame(3, 40);
        TabsState tabs({"First", "Second", "Third"}, 0);
        Tabs(tabs).draw(frame, frame.area());

        // Check tabs were rendered
        assert(tabs.rendered_tabs.size() == 3);

        // Click on "Second" tab
        const auto& second_tab = tabs.rendered_tabs[1];
        MouseEvent click_second{
            .row = second_tab.start_row,
            .col = second_tab.start_col + 1,
            .button = MouseButton::Left,
            .action = MouseAction::Press,
            .shift = false,
            .alt = false,
            .ctrl = false
        };
        bool handled = tabs.handle_mouse(click_second);
        assert(handled);
        assert(tabs.selected == 1);

        // Wheel Down cycles to "Third"
        MouseEvent wheel_down{
            .row = 0,
            .col = 0,
            .button = MouseButton::WheelDown,
            .action = MouseAction::Press,
            .shift = false,
            .alt = false,
            .ctrl = false
        };
        handled = tabs.handle_mouse(wheel_down);
        assert(handled);
        assert(tabs.selected == 2);

        // Wheel Up cycles back to "Second"
        MouseEvent wheel_up{
            .row = 0,
            .col = 0,
            .button = MouseButton::WheelUp,
            .action = MouseAction::Press,
            .shift = false,
            .alt = false,
            .ctrl = false
        };
        handled = tabs.handle_mouse(wheel_up);
        assert(handled);
        assert(tabs.selected == 1);
    }

    // 2. ListState direct mouse click & wheel
    {
        Frame frame(6, 20);
        ListState list({"Option 0", "Option 1", "Option 2", "Option 3"});
        List(list).border_style(BorderStyle::Single).draw(frame, frame.area());

        // Rendered inner should be row 1..4, col 1..18
        assert(list.rendered_inner.row == 1);
        assert(list.selected == 0);

        // Click on Option 2 (row 3)
        MouseEvent click_opt2{
            .row = 3,
            .col = 5,
            .button = MouseButton::Left,
            .action = MouseAction::Press,
            .shift = false,
            .alt = false,
            .ctrl = false
        };
        bool handled = list.handle_mouse(click_opt2);
        assert(handled);
        assert(list.selected == 2);

        // Wheel Up selects Option 1
        MouseEvent wheel_up{
            .row = 3,
            .col = 5,
            .button = MouseButton::WheelUp,
            .action = MouseAction::Press,
            .shift = false,
            .alt = false,
            .ctrl = false
        };
        handled = list.handle_mouse(wheel_up);
        assert(handled);
        assert(list.selected == 1);

        // Wheel Down selects Option 2
        MouseEvent wheel_down{
            .row = 3,
            .col = 5,
            .button = MouseButton::WheelDown,
            .action = MouseAction::Press,
            .shift = false,
            .alt = false,
            .ctrl = false
        };
        handled = list.handle_mouse(wheel_down);
        assert(handled);
        assert(list.selected == 2);
    }

    // 3. FocusManager integration: single click focuses AND selects sub-item
    {
        App app;
        FocusManager fm;

        TabsState category_tabs({"General", "Security", "Billing"}, 0);
        ListState role_list({"Junior", "Mid", "Senior", "Lead"});

        fm.add(category_tabs).add(role_list);
        app.use_focus(fm);

        // Initial state: category_tabs is focused, selected = 0
        assert(fm.is_focused(category_tabs));
        assert(!fm.is_focused(role_list));
        assert(category_tabs.selected == 0);
        assert(role_list.selected == 0);

        // Render both to establish hit buffer and geometry
        Frame& frame = app.frame();
        frame = Frame(12, 30);
        Area tabs_area(0, 0, 30, 2);
        Area list_area(2, 0, 30, 6);

        Tabs(category_tabs).draw(frame, tabs_area);
        List(role_list).border_style(BorderStyle::Single).draw(frame, list_area);

        // Click on list item "Senior" (inner row is 3. Row 3 = "Junior", Row 4 = "Mid", Row 5 = "Senior")
        MouseEvent click_senior{
            .row = 5,
            .col = 5,
            .button = MouseButton::Left,
            .action = MouseAction::Press,
            .shift = false,
            .alt = false,
            .ctrl = false
        };
        app.dispatch_event(Event{click_senior});

        // Verifications:
        // 1. Focus shifted to role_list!
        assert(!fm.is_focused(category_tabs));
        assert(fm.is_focused(role_list));
        // 2. Specific item "Senior" (index 2) was selected in the same click!
        assert(role_list.selected == 2);

        // Mouse wheel over category_tabs (row 0, col 5) scrolls tabs even without switching focus
        MouseEvent wheel_tabs{
            .row = 0,
            .col = 5,
            .button = MouseButton::WheelDown,
            .action = MouseAction::Press,
            .shift = false,
            .alt = false,
            .ctrl = false
        };
        app.dispatch_event(Event{wheel_tabs});
        assert(category_tabs.selected == 1); // Switched to "Security"!
    }

    std::cout << "PASSED\n";
}

void test_modal_mouse_focus_transition()
{
    std::cout << "[Test 7] Modal popup focus transition on mouse click & repeat cycles... ";

    App app;
    FocusManager fm;

    ListState files_list({"main.cpp", "core.cpp"});
    TextAreaState editor_state("void main() {}");
    const Id id_btn_delete("btn_delete");

    TextInputState delete_confirm_input;
    const Id id_modal_cancel("modal_cancel");

    enum class ActiveModal { None, DeleteConfirm };
    ActiveModal active_modal = ActiveModal::None;

    auto close_modal = [&]() {
        if (active_modal != ActiveModal::None) {
            fm.pop_scope();
            active_modal = ActiveModal::None;
        }
    };

    fm.add(files_list)
      .add(editor_state)
      .add(id_btn_delete, [&](const KeyEvent& k) {
          if (k.is(Key::Enter)) {
              active_modal = ActiveModal::DeleteConfirm;
              delete_confirm_input.clear();
              fm.push_scope(Id("delete_modal"), ScopeMode::Modal);
              fm.add(delete_confirm_input).add(id_modal_cancel);
              return true;
          }
          return false;
      });

    app.use_focus(fm);

    app.on_render([&](Frame& frame) {
        Box().id(id_btn_delete).draw(frame, Area(10, 10, 20, 3));
        TextArea(editor_state).draw(frame, Area(0, 0, 40, 10));

        if (active_modal == ActiveModal::DeleteConfirm) {
            Area modal_area(5, 5, 30, 8);
            Box().draw(frame, modal_area);
            TextInput(delete_confirm_input).draw(frame, Area(6, 6, 25, 3));
            Box().id(id_modal_cancel).draw(frame, Area(9, 6, 12, 3));
        }
    });

    app.frame() = Frame(24, 80);
    app.draw_frame(false);

    // Initial state: files_list is focused. Switch focus to editor_state.
    fm.focus(editor_state);
    app.draw_frame(false);
    assert(fm.is_focused(editor_state));
    assert(editor_state.focused);

    MouseEvent click_delete{
        .row = 11,
        .col = 11,
        .button = MouseButton::Left,
        .action = MouseAction::Press,
        .shift = false,
        .alt = false,
        .ctrl = false
    };

    for (int cycle = 1; cycle <= 3; ++cycle)
    {
        // Click on delete button with mouse:
        app.dispatch_event(Event{click_delete});
        app.draw_frame(false);

        // Verification 1: Modal is now open
        assert(active_modal == ActiveModal::DeleteConfirm);

        // Verification 2: Focus shifted IMMEDIATELY to modal popup input!
        assert(!fm.is_focused(editor_state));
        assert(fm.is_focused(delete_confirm_input));
        assert(!editor_state.focused);
        assert(delete_confirm_input.focused);

        // Close modal via Escape
        close_modal();
        app.draw_frame(false);

        // Verification 3: Focus restored back to button that opened modal
        assert(active_modal == ActiveModal::None);
        assert(fm.is_focused(id_btn_delete));
    }

    std::cout << "PASSED\n";
}

void test_h2_hidden_widget_tab_focus_skipping_and_removal()
{
    std::cout << "[Test 8] H2/M5: Hidden widget tab skipping & dynamic target unregistration... ";

    Frame frame(20, 60);

    TextInputState input_tab1_a("First");
    TextInputState input_tab1_b("Second");
    TextInputState input_tab2_hidden("HiddenOnTab2");

    // Render tab 1 widgets only
    Area a1(1, 1, 20, 3);
    Area a2(5, 1, 20, 3);
    TextInput(input_tab1_a).draw(frame, a1);
    TextInput(input_tab1_b).draw(frame, a2);
    // input_tab2_hidden is NOT drawn on frame (simulating inactive tab page)

    FocusManager fm;
    fm.add(input_tab1_a)
      .add(input_tab2_hidden)
      .add(input_tab1_b);

    fm.sync_geometry(frame);

    // Verify geometry harvested
    assert(fm.has_geometry());
    assert(frame.bounds_of(Id(&input_tab1_a)).has_value());
    assert(frame.bounds_of(Id(&input_tab1_b)).has_value());
    assert(!frame.bounds_of(Id(&input_tab2_hidden)).has_value());

    // 1. Focus input_tab1_a
    bool foc_a = fm.focus(input_tab1_a);
    assert(foc_a);
    assert(fm.is_focused(input_tab1_a));
    assert(input_tab1_a.focused);
    assert(!input_tab1_b.focused);
    assert(!input_tab2_hidden.focused);

    // 2. Tab forward (next): MUST skip hidden widget and land on input_tab1_b!
    bool moved = fm.next();
    assert(moved);
    assert(fm.is_focused(input_tab1_b));
    assert(input_tab1_b.focused);
    assert(!input_tab1_a.focused);
    assert(!input_tab2_hidden.focused);

    // 3. Tab forward again: MUST wrap around to input_tab1_a, never landing on hidden widget!
    bool wrapped = fm.next();
    assert(wrapped);
    assert(fm.is_focused(input_tab1_a));
    assert(input_tab1_a.focused);
    assert(!input_tab2_hidden.focused);

    // 4. Shift+Tab backward (prev): MUST land on input_tab1_b, skipping hidden widget
    bool moved_back = fm.prev();
    assert(moved_back);
    assert(fm.is_focused(input_tab1_b));
    assert(input_tab1_b.focused);
    assert(!input_tab2_hidden.focused);

    // 5. Dynamic visibility disablement
    fm.set_visible(Id(&input_tab1_b), false);
    assert(!fm.is_visible(Id(&input_tab1_b)));
    // When input_tab1_b is hidden, next() should stay on/move to input_tab1_a
    fm.next();
    assert(fm.is_focused(input_tab1_a));

    // 6. Dynamic unregistration / removal (Fixes M5)
    size_t count_before = fm.size();
    bool removed = fm.remove(input_tab1_b);
    assert(removed);
    assert(fm.size() == count_before - 1);
    assert(!fm.focus(input_tab1_b));

    std::cout << "PASSED\n";
}

int main()
{
    std::cout << "========================================\n";
    std::cout << " Running koto Headless Focus & Events Tests\n";
    std::cout << "========================================\n";

    test_kitty_keyboard_and_paste();
    test_hit_buffer_and_box_focus();
    test_universal_id_and_focus_manager();
    test_spatial_navigation_and_modal_isolation();
    test_app_use_focus_and_mouse();
    test_subitem_mouse_interaction();
    test_modal_mouse_focus_transition();
    test_h2_hidden_widget_tab_focus_skipping_and_removal();

    std::cout << "========================================\n";
    std::cout << " ALL 8 FOCUS & EVENT TESTS PASSED CLEANLY!\n";
    std::cout << "========================================\n";
    return 0;
}
