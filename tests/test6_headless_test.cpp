#include <koto/koto.hpp>
#include <cassert>
#include <iostream>
#include <string>

using namespace koto;

void test_box_title_alignment()
{
    std::cout << "[Test 1] Box title alignment... ";
    Frame frame(5, 20); // Headless 5 rows x 20 columns

    Box()
        .border_style(BorderStyle::Rounded)
        .title("Files")
        .title_align(TitleAlign::Left)
        .draw(frame, frame.area());

    std::string top_row = frame.to_plain_text_row(0);
    // Left title should start with ╭─ Files ─
    assert(top_row.rfind("╭─ Files ─", 0) == 0);

    // Test Center title
    frame.clear_back_buffer();
    Box()
        .border_style(BorderStyle::Rounded)
        .title("Files")
        .title_align(TitleAlign::Center)
        .draw(frame, frame.area());

    std::string center_top = frame.to_plain_text_row(0);
    // Centered title has " Files " roughly in the middle
    assert(center_top.find(" Files ") != std::string::npos);
    assert(center_top.find(" Files ") > 3);

    std::cout << "PASSED\n";
}

void test_text_hard_newlines()
{
    std::cout << "[Test 2] Text hard newlines without soft-wrap... ";
    Frame frame(5, 20);

    std::string multi = "Line 1\nLine 2\nLine 3";
    Text(multi).draw(frame, frame.area());

    assert(frame.to_plain_text_row(0) == "Line 1");
    assert(frame.to_plain_text_row(1) == "Line 2");
    assert(frame.to_plain_text_row(2) == "Line 3");
    assert(frame.to_plain_text_row(3).empty());

    std::cout << "PASSED\n";
}

void test_list_highlight_symbol()
{
    std::cout << "[Test 3] List highlight symbol & alignment... ";
    Frame frame(4, 20);

    ListState state({"alpha", "beta"});
    state.selected = 0;

    List(state)
        .highlight_symbol("> ")
        .draw(frame, frame.area());

    // Row 0 is selected: should have "> alpha"
    assert(frame.to_plain_text_row(0) == "> alpha");
    // Row 1 is unselected: should have "  beta" (aligned)
    assert(frame.to_plain_text_row(1) == "  beta");

    std::cout << "PASSED\n";
}

void test_list_full_width_bg()
{
    std::cout << "[Test 4] List full-width row highlight background... ";
    Frame frame(3, 10);

    ListState state({"cat"});
    state.selected = 0;

    uint32_t highlight_bg = parse_color("#003366");
    List(state)
        .highlight_bg(highlight_bg)
        .draw(frame, frame.area());

    // Check that every column from 0 to 9 on row 0 has highlight_bg
    for (int col = 0; col < 10; col++)
    {
        Cell cell = frame.get_cell(0, col);
        assert(cell.bg == highlight_bg);
    }

    std::cout << "PASSED\n";
}

void test_layout_clamping()
{
    std::cout << "[Test 5] Layout boundary clamping under-size protection... ";
    Area tiny(0, 0, 10, 5); // 5 rows tall

    // Requesting 25 rows in a 5-row area
    auto rows = Layout::vertical(tiny, {
        Constraint::fixed(10),
        Constraint::fixed(10),
        Constraint::fixed(5),
    });

    assert(rows.size() == 3);
    assert(rows[0].height == 5); // Clamped to remaining (5 - 0 = 5)
    assert(rows[1].height == 0); // Clamped to remaining (5 - 5 = 0)
    assert(rows[2].height == 0); // Clamped to remaining (5 - 5 = 0)

    // Ensure none of the rects exceed area.bottom()
    for (const auto& r : rows)
    {
        assert(r.bottom() <= tiny.bottom());
    }

    std::cout << "PASSED\n";
}

void test_list_title_and_overflow()
{
    std::cout << "[Test 6] List title and text overflow modes... ";
    Frame frame(5, 20);

    ListState state({"super_extra_long_item_name_exceeding_width"});
    state.selected = 0;

    // Test with title and Ellipsis overflow (default)
    List(state)
        .border_style(BorderStyle::Rounded)
        .title("Items")
        .title_align(TitleAlign::Left)
        .draw(frame, frame.area());

    // Row 0 has the rounded border with title
    std::string top_row = frame.to_plain_text_row(0);
    assert(top_row.rfind("╭─ Items ─", 0) == 0);

    // Row 1 inside the box has the item with an ellipsis
    std::string item_row = frame.to_plain_text_row(1);
    assert(item_row.find("…") != std::string::npos);

    // Test with Clip overflow
    frame.clear_back_buffer();
    List(state)
        .border_style(BorderStyle::None)
        .overflow(TextOverflow::Clip)
        .draw(frame, frame.area());

    std::string clipped_row = frame.to_plain_text_row(0);
    assert(clipped_row.find("…") == std::string::npos);
    assert(clipped_row.rfind("super_extra_long_ite", 0) == 0);

    std::cout << "PASSED\n";
}

void test_color_rgb_and_lerp()
{
    std::cout << "[Test 7] Color::rgb, Color::Rgb, and Color::lerp... ";

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

void test_text_alignment()
{
    std::cout << "[Test 8] Text alignment (Center & Right)... ";
    Frame frame(3, 20);

    // Centered text: "Center" in width 20 -> (20 - 6) / 2 = 7 spaces before
    Text("Center")
        .align(TextAlign::Center)
        .draw(frame, frame.area());

    std::string center_row = frame.to_plain_text_row(0);
    assert(center_row.find("Center") == 7);

    // Right-aligned text: "Right" in width 20 -> (20 - 5) = 15 spaces before
    frame.clear_back_buffer();
    Text("Right")
        .align(TextAlign::Right)
        .draw(frame, frame.area());

    std::string right_row = frame.to_plain_text_row(0);
    assert(right_row.find("Right") == 15);

    std::cout << "PASSED\n";
}

void test_rule_divider()
{
    std::cout << "[Test 9] Rule widget (solid & titled divider)... ";
    Frame frame(3, 20);

    // Plain horizontal rule
    Rule().style(BorderStyle::Single).draw(frame, frame.area());
    std::string rule_row = frame.to_plain_text_row(1); // row 1 is vertical center
    assert(rule_row.find("────────────────────") != std::string::npos);

    // Titled rule centered
    frame.clear_back_buffer();
    Rule("Logs")
        .style(BorderStyle::Double)
        .title_align(TitleAlign::Center)
        .draw(frame, frame.area());

    std::string titled_row = frame.to_plain_text_row(1);
    assert(titled_row.find(" Logs ") != std::string::npos);
    assert(titled_row.find("═") != std::string::npos);

    std::cout << "PASSED\n";
}

void test_gauge_and_sparkline()
{
    std::cout << "[Test 10] Gauge (progress bar) & Sparkline... ";
    Frame frame(2, 10);

    // 50% gauge with solid block
    Gauge()
        .percent(50)
        .filled_char(U'#')
        .empty_char(U'.')
        .draw(frame, Area(0, 0, 10, 1));

    std::string gauge_row = frame.to_plain_text_row(0);
    assert(gauge_row == "#####.....");

    // Sparkline with 8 values from 0 to 8 in width 8
    frame.clear_back_buffer();
    std::vector<uint64_t> vals = {1, 2, 3, 4, 5, 6, 7, 8};
    Sparkline()
        .data(vals)
        .max(8)
        .align_right(false)
        .draw(frame, Area(0, 0, 8, 1));

    // The sparkline should render increasing unicode vertical bars
    std::string spark_row = frame.to_plain_text_row(0);
    assert(!spark_row.empty());

    std::cout << "PASSED\n";
}

void test_grid_layout_and_fill()
{
    std::cout << "[Test 11] Layout::grid, grid_uniform, and Frame::fill... ";
    Area screen(0, 0, 30, 15);

    // 3x3 uniform grid (Tic-Tac-Toe): 15 rows / 3 = 5 rows tall, 30 cols / 3 = 10 cols wide
    auto ttt_grid = Layout::grid_uniform(screen, 3, 3);
    assert(ttt_grid.rows() == 3);
    assert(ttt_grid.cols() == 3);
    assert(ttt_grid.size() == 9);

    assert(ttt_grid[0][0].row == 0 && ttt_grid[0][0].col == 0);
    assert(ttt_grid[0][0].height == 5 && ttt_grid[0][0].width == 10);

    assert(ttt_grid[1][1].row == 5 && ttt_grid[1][1].col == 10);
    assert(ttt_grid[1][1].height == 5 && ttt_grid[1][1].width == 10);

    assert(ttt_grid[2][2].row == 10 && ttt_grid[2][2].col == 20);
    assert(ttt_grid[2][2].height == 5 && ttt_grid[2][2].width == 10);

    // 8x8 uniform grid (Chess board) in a 40x16 area
    Area chess_area(0, 0, 40, 16);
    auto chess_grid = Layout::grid_uniform(chess_area, 8, 8);
    assert(chess_grid.rows() == 8);
    assert(chess_grid.cols() == 8);
    assert(chess_grid.size() == 64);

    // Each cell in chess is 16 / 8 = 2 rows high, 40 / 8 = 5 cols wide
    for (size_t r = 0; r < 8; ++r)
    {
        for (size_t c = 0; c < 8; ++c)
        {
            assert(chess_grid[r][c].height == 2);
            assert(chess_grid[r][c].width == 5);
        }
    }

    // Frame::fill test
    Frame frame(5, 10);
    frame.fill(Area(1, 2, 4, 3), U'#', Color::Cyan, Color::Rgb(10, 20, 30));
    for (int r = 1; r < 4; ++r)
    {
        for (int c = 2; c < 6; ++c)
        {
            Cell cell = frame.get_cell(r, c);
            assert(cell.ch == U'#');
            assert(cell.fg == Color::Cyan);
            assert(cell.bg == Color::Rgb(10, 20, 30));
        }
    }

    std::cout << "PASSED\n";
}

void test_text_area()
{
    std::cout << "[Test 12] TextAreaState & TextArea rendering (sticky cursor, newline, backspace, gutter)... ";

    // 1. Text initialization & line splitting
    TextAreaState state("Hello World\nSecond Line\nThird Line");
    assert(state.text.size() == 3);
    assert(state.text[0] == "Hello World");
    assert(state.text[1] == "Second Line");
    assert(state.text[2] == "Third Line");
    assert(state.cursor_row == 0);
    assert(state.cursor_col == 0);

    // 2. newline() without auto-indentation
    // Cursor at col 5 ("Hello")
    state.cursor_col = 5;
    state.newline();
    assert(state.text.size() == 4);
    assert(state.text[0] == "Hello");
    assert(state.text[1] == " World");
    assert(state.cursor_row == 1);
    assert(state.cursor_col == 0);

    // 3. backspace() at col 0 joins lines
    state.backspace();
    assert(state.text.size() == 3);
    assert(state.text[0] == "Hello World");
    assert(state.cursor_row == 0);
    assert(state.cursor_col == 5);

    // 4. delete_forward() at end of line pulls next line up
    state.move_end();
    assert(state.cursor_col == 11);
    state.delete_forward();
    assert(state.text.size() == 2);
    assert(state.text[0] == "Hello WorldSecond Line");
    assert(state.text[1] == "Third Line");

    // 5. Sticky Cursor Navigation Mode
    TextAreaState sticky_state(std::vector<std::string>{"1234567890", "ab", "1234567890"});
    sticky_state.cursor_mode = CursorColumnMode::Sticky;
    sticky_state.cursor_row = 0;
    sticky_state.cursor_col = 8;
    sticky_state.preferred_col = 8;

    sticky_state.move_down();
    assert(sticky_state.cursor_row == 1);
    assert(sticky_state.cursor_col == 2); // Clamped on short line
    assert(sticky_state.preferred_col == 8); // Goal preserved

    sticky_state.move_down();
    assert(sticky_state.cursor_row == 2);
    assert(sticky_state.cursor_col == 8); // Restored on long line!

    // 6. Clamp Cursor Navigation Mode
    TextAreaState clamp_state(std::vector<std::string>{"1234567890", "ab", "1234567890"});
    clamp_state.cursor_mode = CursorColumnMode::Clamp;
    clamp_state.cursor_row = 0;
    clamp_state.cursor_col = 8;
    clamp_state.preferred_col = 8;

    clamp_state.move_down();
    assert(clamp_state.cursor_row == 1);
    assert(clamp_state.cursor_col == 2); // Clamped on short line
    assert(clamp_state.preferred_col == 2); // Goal collapsed in Clamp mode

    clamp_state.move_down();
    assert(clamp_state.cursor_row == 2);
    assert(clamp_state.cursor_col == 2); // Stays at 2 in Clamp mode

    // 7. Rendering & Line Numbers Gutter
    Frame frame(5, 20);
    TextAreaState view_state("Alpha\nBeta\nGamma");
    TextArea(view_state)
        .border_style(BorderStyle::None)
        .line_numbers(true)
        .draw(frame, Area(0, 0, 20, 5));

    // Line 1: " 1 │Alpha"
    std::string row0 = frame.to_plain_text_row(0);
    assert(row0.find("1") != std::string::npos);
    assert(row0.find("│") != std::string::npos);
    assert(row0.find("Alpha") != std::string::npos);

    // Hardware cursor check
    assert(frame.cursor_row() == 0);
    // Gutter is "  1 │" (4 cols), so cursor should be at column 4
    assert(frame.cursor_col() == 4);

    std::cout << "PASSED\n";
}

void test_tabs()
{
    std::cout << "[Test 13] TabsState & Tabs widget (navigation, wrapping, styles)... ";

    // 1. TabsState navigation & wrapping
    TabsState state({"Code", "Files", "Terminal", "Settings"});
    assert(state.titles.size() == 4);
    assert(state.selected == 0);
    assert(*state.selected_title() == "Code");

    state.select_next();
    assert(state.selected == 1);
    assert(*state.selected_title() == "Files");

    state.select_prev();
    assert(state.selected == 0);

    // Wrapping backwards from 0 -> last (3)
    state.select_prev();
    assert(state.selected == 3);
    assert(*state.selected_title() == "Settings");

    // Wrapping forwards from last -> 0
    state.select_next();
    assert(state.selected == 0);

    // Number key switching: '2' -> index 1
    KeyEvent key2{};
    key2.ch = '2';
    state.handle_event(key2);
    assert(state.selected == 1);

    // 2. TabsStyle::Bar rendering
    Frame frame_bar(1, 40);
    Tabs(state)
        .style(TabsStyle::Bar)
        .separator("│")
        .draw(frame_bar, Area(0, 0, 40, 1));

    std::string bar_row = frame_bar.to_plain_text_row(0);
    assert(bar_row.find("Code") != std::string::npos);
    assert(bar_row.find("│") != std::string::npos);
    assert(bar_row.find("Files") != std::string::npos);

    // 3. TabsStyle::Pill rendering
    Frame frame_pill(1, 40);
    Tabs(state)
        .style(TabsStyle::Pill)
        .draw(frame_pill, Area(0, 0, 40, 1));

    std::string pill_row = frame_pill.to_plain_text_row(0);
    // Active tab (Files) should be wrapped in "[ Files ]"
    assert(pill_row.find("[ Files ]") != std::string::npos);

    // 4. TabsStyle::Underline rendering
    Frame frame_underline(2, 40);
    Tabs(state)
        .style(TabsStyle::Underline)
        .draw(frame_underline, Area(0, 0, 40, 2));

    std::string u_row0 = frame_underline.to_plain_text_row(0);
    std::string u_row1 = frame_underline.to_plain_text_row(1);
    assert(u_row0.find("Files") != std::string::npos);
    assert(u_row1.find("━") != std::string::npos);
    assert(u_row1.find("─") != std::string::npos);

    std::cout << "PASSED\n";
}

void test_kitty_keyboard_and_paste()
{
    std::cout << "[Test 14] Kitty keyboard protocol (CSI u) & Bracketed paste mode... ";

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

void test_cpp23_modernizations()
{
    std::cout << "[Test 15] C++23 native features (operator[r, c], std::expected, std::views::enumerate)... ";

    // 1. Frame::operator[r, c] 2D indexing & mutation
    {
        Frame frame(4, 10);
        frame[1, 2] = Cell{U'★', Color::Yellow, Color::Black};
        assert(frame.get_cell(1, 2).ch == U'★');
        assert(frame[1, 2].ch == U'★');
        assert(frame[1, 2].fg == Color::Yellow);

        const Frame& cf = frame;
        assert(cf[1, 2].ch == U'★');
        assert(cf[0, 0].ch == U' '); // default empty cell
    }

    // 2. Grid::operator[r, c] 2D indexing
    {
        Area root(0, 0, 80, 24);
        auto grid = Layout::grid_uniform(root, 3, 4);
        Area cell_1_2 = grid[1, 2];
        assert(cell_1_2 == grid.at(1, 2));
        assert(cell_1_2 == grid(1, 2));
        assert(cell_1_2 == grid[1][2]); // backward compatible proxy
    }

    // 3. std::expected Color parsing (try_parse_color)
    {
        auto ok_6 = try_parse_color("#FF5500");
        assert(ok_6.has_value());
        assert(*ok_6 == 0x00FF5500);

        auto ok_3 = try_parse_color("#F00");
        assert(ok_3.has_value());
        assert(*ok_3 == Color::Red);

        auto err_empty = try_parse_color("");
        assert(!err_empty.has_value());
        assert(err_empty.error() == ColorParseError::EmptyString);

        auto err_len = try_parse_color("#1234");
        assert(!err_len.has_value());
        assert(err_len.error() == ColorParseError::InvalidLength);

        auto err_char = try_parse_color("#ZZZZZZ");
        assert(!err_char.has_value());
        assert(err_char.error() == ColorParseError::InvalidHexDigit);
    }

    // 4. TextAreaState::to_string with std::views::enumerate
    {
        TextAreaState ta("alpha\nbeta\ngamma");
        assert(ta.to_string() == "alpha\nbeta\ngamma");
    }

    std::cout << "PASSED\n";
}

void test_zero_overhead_modernizations()
{
    std::cout << "[Test 16] Zero-overhead modernizations (string_view stream decoding, optional returns, null-safe State&)... ";

    // 1. UTF-8 next_codepoint(std::string_view&) stream decoding
    {
        std::string_view stream = "A★\xF0\x9F\x8C\x9F" "B"; // 'A', U'★', U'🌟' (U+1F31F), 'B'
        char32_t cp1 = next_codepoint(stream);
        assert(cp1 == U'A');
        char32_t cp2 = next_codepoint(stream);
        assert(cp2 == U'★');
        char32_t cp3 = next_codepoint(stream);
        assert(cp3 == 0x1F31F);
        char32_t cp4 = next_codepoint(stream);
        assert(cp4 == U'B');
        assert(stream.empty());
    }

    // 2. UTF-8 encode_utf8(char32_t) returning Utf8Buffer value
    {
        auto buf1 = encode_utf8(U'A');
        assert(buf1.size == 1);
        assert(buf1.view() == "A");

        auto buf2 = encode_utf8(U'★');
        assert(buf2.size == 3);
        assert(buf2.view() == "★");

        auto buf3 = encode_utf8(0x1F31F);
        assert(buf3.size == 4);
        assert(buf3.view() == "\xF0\x9F\x8C\x9F");
    }

    // 3. ListState::selected_item() returning std::optional<std::string_view>
    {
        ListState empty_state;
        assert(!empty_state.selected_item().has_value());
        assert(empty_state.selected_item_ptr() == nullptr);

        ListState state({"main.cpp", "include/", "CMakeLists.txt"});
        state.selected = 1;
        auto item = state.selected_item();
        assert(item.has_value());
        assert(*item == "include/");
        assert(item.value() == "include/");
        assert(state.selected_item_ptr() != nullptr && *state.selected_item_ptr() == "include/");
    }

    // 4. TabsState::selected_title() returning std::optional<std::string_view>
    {
        TabsState empty_tabs;
        assert(!empty_tabs.selected_title().has_value());
        assert(empty_tabs.selected_title_ptr() == nullptr);

        TabsState tabs({"Editor", "Terminal", "Output"}, 0);
        auto title = tabs.selected_title();
        assert(title.has_value());
        assert(*title == "Editor");
        assert(tabs.selected_title_ptr() != nullptr && *tabs.selected_title_ptr() == "Editor");
    }

    // 5. TextInput & List null-safe references
    {
        Frame frame(3, 20);
        TextInputState tis("hello world");
        TextInput ti(tis);
        ti.draw(frame, frame.area());

        ListState ls({"Item 1", "Item 2"});
        List l(ls);
        l.draw(frame, frame.area());
    }

    std::cout << "PASSED\n";
}

void test_hit_buffer_and_box_focus()
{
    std::cout << "[Test 17] Frame hit-buffer (-1 default tag) & Box visual focus... ";

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
    std::cout << "[Test 18] Universal Id token & FocusManager (Tab cycle, key routing, mouse click)... ";

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
    std::cout << "[Test 19] Spatial 2D navigation (auto-harvested bounds) & Modal focus trapping... " << std::flush;

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
    std::cout << "[Test 20] App::use_focus(fm) integration & SGR mouse parser... " << std::flush;

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

void test_audit_fixes_and_edge_cases()
{
    std::cout << "[Test 21] Full audit fixes (List/Tabs focus defaults, auto-tagging, Layout::center clamping, UTF-8 safety)... ";

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

void test_subitem_mouse_interaction()
{
    std::cout << "[Test 22] Sub-item mouse click & wheel interactions (List, Tabs, FocusManager)... ";

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
    std::cout << "[Test 23] Modal popup focus transition on mouse click & repeat cycles... ";

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

int main()
{
    std::cout << "========================================\n";
    std::cout << " Running koto Headless Snapshot Tests\n";
    std::cout << "========================================\n";

    test_box_title_alignment();
    test_text_hard_newlines();
    test_list_highlight_symbol();
    test_list_full_width_bg();
    test_layout_clamping();
    test_list_title_and_overflow();
    test_color_rgb_and_lerp();
    test_text_alignment();
    test_rule_divider();
    test_gauge_and_sparkline();
    test_grid_layout_and_fill();
    test_text_area();
    test_tabs();
    test_kitty_keyboard_and_paste();
    test_cpp23_modernizations();
    test_zero_overhead_modernizations();
    test_hit_buffer_and_box_focus();
    test_universal_id_and_focus_manager();
    test_spatial_navigation_and_modal_isolation();
    test_app_use_focus_and_mouse();
    test_audit_fixes_and_edge_cases();
    test_subitem_mouse_interaction();
    test_modal_mouse_focus_transition();

    std::cout << "========================================\n";
    std::cout << " ALL 23 HEADLESS TESTS PASSED CLEANLY!\n";
    std::cout << "========================================\n";
    return 0;
}
