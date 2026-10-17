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

    std::cout << "========================================\n";
    std::cout << " ALL 13 HEADLESS TESTS PASSED CLEANLY!\n";
    std::cout << "========================================\n";
    return 0;
}
