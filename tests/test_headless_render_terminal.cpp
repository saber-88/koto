#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <koto/koto.hpp>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <unistd.h>

using namespace koto;

void test_color_rgb_and_lerp()
{
    std::cout << "[Test 1] Color::rgb, Color::Rgb, and Color::lerp... ";

    uint32_t c1 = Color::rgb(255, 0, 0); // Red
    uint32_t c2 = Color::Rgb(0, 255, 0); // Green
    assert(c1 == Color::Red);
    assert(c2 == Color::Green);

    // Lerp midway between red (255, 0, 0) and green (0, 255, 0) should be (128, 128, 0)
    uint32_t mid = Color::lerp(c1, c2, 0.5f);
    uint8_t r = static_cast<uint8_t>((mid >> 16) & 0xFF);
    uint8_t g = static_cast<uint8_t>((mid >> 8) & 0xFF);
    uint8_t b = static_cast<uint8_t>(mid & 0xFF);
    assert(r == 128);
    assert(g == 128);
    assert(b == 0);

    // Bounds checking
    assert(Color::lerp(c1, c2, 0.0f) == c1);
    assert(Color::lerp(c1, c2, 1.0f) == c2);

    std::cout << "PASSED\n";
}

void test_color_mode_and_quantization()
{
    std::cout << "[Test 2] Color capability downscaling (256, 16, NO_COLOR)... ";

    // 1. Test rgb_to_ansi256
    // Pure black -> 16
    assert(rgb_to_ansi256(0, 0, 0) == 16);
    // Pure red -> 196 (16 + 36*5 + 0 + 0)
    assert(rgb_to_ansi256(255, 0, 0) == 196);
    // Pure green -> 46 (16 + 0 + 6*5 + 0)
    assert(rgb_to_ansi256(0, 255, 0) == 46);
    // Pure blue -> 21 (16 + 0 + 0 + 5)
    assert(rgb_to_ansi256(0, 0, 255) == 21);
    // Grayscale ramp
    assert(rgb_to_ansi256(128, 128, 128) >= 232 && rgb_to_ansi256(128, 128, 128) <= 255);

    // 2. Test rgb_to_ansi16
    // Pure red -> 9 (Bright Red)
    assert(rgb_to_ansi16(255, 0, 0) == 9);
    // Dark red -> 1 (Standard Red)
    assert(rgb_to_ansi16(128, 0, 0) == 1);
    // Pure blue -> 12 (Bright Blue)
    assert(rgb_to_ansi16(0, 0, 255) == 12);
    // Pure black -> 0
    assert(rgb_to_ansi16(0, 0, 0) == 0);

    // 3. Test Frame rendering with different ColorMode configurations
    Terminal term;

    // Test TrueColor mode
    term.color_mode(ColorMode::TrueColor);
    Frame frame_true(term);
    frame_true[0, 0].ch = U'X';
    frame_true[0, 0].fg = Color::rgb(255, 128, 0);
    frame_true.render(false);
    std::string_view out_true = frame_true.raw_output();
    assert(out_true.find("38;2;255;128;0m") != std::string_view::npos);

    // Test Ansi256 mode
    term.color_mode(ColorMode::Ansi256);
    Frame frame_256(term);
    frame_256[0, 0].ch = U'X';
    frame_256[0, 0].fg = Color::rgb(255, 0, 0); // 196
    frame_256.render(false);
    std::string_view out_256 = frame_256.raw_output();
    assert(out_256.find("38;5;196m") != std::string_view::npos);

    // Test Ansi16 mode
    term.color_mode(ColorMode::Ansi16);
    Frame frame_16(term);
    frame_16[0, 0].ch = U'X';
    frame_16[0, 0].fg = Color::rgb(255, 0, 0); // index 9 -> SGR 91m
    frame_16.render(false);
    std::string_view out_16 = frame_16.raw_output();
    assert(out_16.find("\033[91m") != std::string_view::npos);

    // Test NoColor mode
    term.color_mode(ColorMode::NoColor);
    Frame frame_nocolor(term);
    frame_nocolor[0, 0].ch = U'X';
    frame_nocolor[0, 0].fg = Color::rgb(255, 0, 0);
    frame_nocolor.render(false);
    std::string_view out_nocolor = frame_nocolor.raw_output();
    assert(out_nocolor.find("38;2;") == std::string_view::npos);
    assert(out_nocolor.find("38;5;") == std::string_view::npos);
    assert(out_nocolor.find("[91m") == std::string_view::npos);

    std::cout << "PASSED\n";
}

void test_semantic_contract_and_color_type()
{
    std::cout << "[Test 3] Semantic contract and Color value type... ";

    // 1. Color value type conversions
    Color c1 = 0x00FF0000;
    Color c2 = "#FF0000";
    Color c3 = "red";
    Color c4 = Color::Red;

    assert(c1.value == 0x00FF0000);
    assert(c2.value == 0x00FF0000);
    assert(c3.value == 0x00FF0000);
    assert(c4.value == 0x00FF0000);
    assert(c1 == c2);
    assert(c2 == c3);
    assert(c3 == c4);

    // Implicit conversion to uint32_t for Cell rendering
    uint32_t raw_val = c1;
    assert(raw_val == 0x00FF0000);

    // 2. Geometry noun accessors on Frame
    Frame frame(24, 80);
    assert(frame.rows() == 24);
    assert(frame.cols() == 80);
    assert(frame.height() == 24);
    assert(frame.width() == 80);

    // 3. Focus API unification
    FocusManager fm;
    Id b1("btn1");
    Id b2("btn2");
    fm.add(b1);
    fm.add(b2);

    assert(fm.focus(b2));
    assert(fm.current_id() == b2);
    assert(fm.focus(static_cast<size_t>(0)));
    assert(fm.current_id() == b1);

    // 4. Widget Color consistency across entire suite
    Area area{0, 0, 20, 5};
    Box().border_style(BorderStyle::Rounded).fg("#FF00FF").bg(Color::Black).draw(frame, area);
    Text("Unified").fg(Color::Cyan).bg(0x00222222).draw(frame, area);
    Fill(Color::Blue).fg(Color::White).draw(frame, Area{0, 0, 5, 2});
    Rule().fg("#00FF00").draw(frame, Area{2, 0, 20, 1});
    Gauge().ratio(0.5f).fg(Color::Green).bg(Color::DarkGray).draw(frame, Area{3, 0, 20, 1});
    std::vector<uint64_t> sp_data = {1, 3, 5, 2};
    Sparkline().data(sp_data).fg("#FFAA00").draw(frame, Area{4, 0, 20, 1});

    Button("Click").fg(Color::Yellow).bg(Color::Black).focused_border_fg("#00FFFF").draw(frame, Area{0, 0, 10, 3});

    TextInputState tis("Hello");
    TextInput(tis).fg(Color::White).bg(Color::Black).placeholder_fg("#888888").draw(frame, Area{0, 0, 20, 3});

    ListState ls({"item 1", "item 2"});
    List(ls).fg(Color::White).highlight_fg("#00FF00").draw(frame, Area{0, 0, 15, 4});

    TextAreaState tas("Multiline\nText");
    TextArea(tas).fg(Color::LightGray).border_style(BorderStyle::Single).draw(frame, Area{0, 0, 20, 4});

    std::vector<std::string> tab_titles = {"Tab1", "Tab2"};
    Tabs(tab_titles).active_fg(Color::DarkGray).inactive_fg(Color::Cyan).draw(frame, Area{0, 0, 20, 3});

    std::cout << "PASSED\n";
}

void test_frame_abstractions_fill_and_mouse_target()
{
    std::cout << "[Test 4] Frame abstractions: Fill, Clear, app.area(), and mouse.target... ";

    // 1. Fill widget test
    Frame frame(5, 10);
    Area fill_area(1, 2, 4, 3);
    Fill().bg(Color::Cyan).fg(Color::Yellow).char_code(U'#').id(Id("fill_test")).draw(frame, fill_area);

    for (int r = 1; r < 4; ++r)
    {
        for (int c = 2; c < 6; ++c)
        {
            Cell cell = frame.get_cell(r, c);
            assert(cell.ch == U'#');
            assert(cell.bg == Color::Cyan);
            assert(cell.fg == Color::Yellow);
            assert(frame.hit_at(r, c) == Id("fill_test"));
        }
    }

    // 2. Clear widget test
    Clear().draw(frame, Area(1, 2, 2, 2));
    for (int r = 1; r < 3; ++r)
    {
        for (int c = 2; c < 4; ++c)
        {
            Cell cell = frame.get_cell(r, c);
            assert(cell.ch == U' ');
            assert(cell.bg == COLOR_DEFAULT);
        }
    }

    // 3. App area and hit_at methods
    App app;
    app.frame().handle_resize(24, 80);
    assert(app.area() == Area(0, 0, 80, 24));

    Area btn_area(5, 10, 15, 3);
    app.frame().tag_area(btn_area, Id("my_btn"));

    MouseEvent test_mouse{};
    test_mouse.row = 6;
    test_mouse.col = 12;
    test_mouse.button = MouseButton::Left;
    test_mouse.action = MouseAction::Press;

    assert(app.hit_at(6, 12) == Id("my_btn"));
    assert(app.hit_at(test_mouse) == Id("my_btn"));

    // 4. MouseEvent automatic target enrichment during dispatch
    Id received_target = Id::none();
    bool received_left_press = false;

    app.on_mouse([&](const MouseEvent& m) {
        received_target = m.target;
        received_left_press = m.is_left_press();
    });

    app.dispatch_event(test_mouse);

    assert(received_target == Id("my_btn"));
    assert(received_left_press);

    std::cout << "PASSED\n";
}

void test_self_pipe_and_wakeup()
{
    std::cout << "[Test 5] POSIX Self-Pipe & thread-safe EventLoop::wakeup... ";

    EventLoop loop;

    // 1. Test thread-safe wakeup from background thread
    auto start_time = std::chrono::steady_clock::now();
    std::thread worker([&loop]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        loop.wakeup();
    });

    // poll with a long timeout (1000ms). The background thread wakeup must interrupt it!
    Event ev = loop.poll(1000);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time
    ).count();

    worker.join();

    // Must return TickEvent and finish well before 1000ms
    assert(ev.is_tick());
    assert(elapsed < 500);

    // 2. Test SIGWINCH signal dispatch through self-pipe
    EventLoop::on_sigwinch(SIGWINCH);
    Event resize_ev = loop.poll(100);
    assert(resize_ev.is_resize());

    // 3. Subsequent poll when pipe is empty should return tick
    Event idle_ev = loop.poll(10);
    assert(idle_ev.is_tick());

    std::cout << "PASSED\n";
}

void test_terminal_error_handling_and_tty()
{
    std::cout << "[Test 6] Terminal TTY detection & std::expected error reporting... ";

    Terminal term;
    // 1. Terminal exposes is_tty() and in_raw_mode()
    bool expected_tty = (isatty(STDIN_FILENO) != 0 && isatty(STDOUT_FILENO) != 0);
    assert(term.is_tty() == expected_tty);

    // 2. Safe dimension fallbacks
    assert(term.get_rows() > 0);
    assert(term.get_cols() > 0);

    // 3. TerminalError message mappings
    TerminalErrorInfo err_notty{.code = TerminalError::NotATty, .system_errno = ENOTTY};
    assert(err_notty.message().find("not a terminal") != std::string_view::npos);

    TerminalErrorInfo err_getattr{.code = TerminalError::GetAttrFailed, .system_errno = EIO};
    assert(err_getattr.message().find("tcgetattr") != std::string_view::npos);

    TerminalErrorInfo err_setattr{.code = TerminalError::SetAttrFailed, .system_errno = EINVAL};
    assert(err_setattr.message().find("tcsetattr") != std::string_view::npos);

    // 4. App::run() returns std::expected<void, TerminalErrorInfo>
    App app;
    if (!term.is_tty())
    {
        // When not a TTY (piped or headless runner), app.run() safely returns unexpected(NotATty)
        auto res = app.run();
        assert(!res.has_value());
        assert(res.error().code == TerminalError::NotATty);
    }

    std::cout << "PASSED\n";
}

int main()
{
    std::cout << "========================================\n";
    std::cout << " Running koto Headless Render & Terminal Tests\n";
    std::cout << "========================================\n";

    test_color_rgb_and_lerp();
    test_color_mode_and_quantization();
    test_semantic_contract_and_color_type();
    test_frame_abstractions_fill_and_mouse_target();
    test_self_pipe_and_wakeup();
    test_terminal_error_handling_and_tty();

    std::cout << "========================================\n";
    std::cout << " ALL 6 RENDER & TERMINAL TESTS PASSED CLEANLY!\n";
    std::cout << "========================================\n";
    return 0;
}
