#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <koto/koto.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace koto;

void test_layout_clamping()
{
    std::cout << "[Test 1] Layout boundary clamping under-size protection... ";
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

void test_grid_layout_and_fill()
{
    std::cout << "[Test 2] Layout::grid, grid_uniform, and Frame::fill... ";
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

void test_cpp23_modernizations()
{
    std::cout << "[Test 3] C++23 native features (operator[r, c], std::expected, std::views::enumerate)... ";

    // 1. Frame::operator[r, c] 2D indexing & mutation
    {
        Frame frame(4, 10);
        frame[1, 2] = Cell{U'★', Color::Yellow, Color::Black};
        assert(frame.get_cell(1, 2).ch == U'★');
        assert((frame[1, 2].ch == U'★'));
        assert((frame[1, 2].fg == Color::Yellow));

        const Frame& cf = frame;
        assert((cf[1, 2].ch == U'★'));
        assert((cf[0, 0].ch == U' ')); // default empty cell
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
    std::cout << "[Test 4] Zero-overhead modernizations (string_view stream decoding, optional returns, null-safe State&)... ";

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

void test_zero_allocation_layout()
{
    std::cout << "[Test 5] Zero-allocation 16-capacity LayoutResult & structured bindings... ";
    Area screen{0, 0, 80, 24};

    // 1. Literal list with structured bindings (deduces std::array<Area, 3> on the stack)
    auto [header, body, footer] = Layout::vertical(screen, {
        Constraint::fixed(3),
        Constraint::fill(),
        Constraint::fixed(1)
    });
    assert(header.height == 3);
    assert(footer.height == 1);
    assert(body.height == 20);

    // 2. auto chunks indexing with LayoutResult (capacity 16 inline)
    auto chunks = Layout::horizontal(screen, {
        Constraint::fixed(10),
        Constraint::fill(),
        Constraint::fixed(10)
    });
    assert(chunks.size() == 3);
    assert(chunks[0].width == 10);
    assert(chunks[2].width == 10);
    assert(chunks[1].width == 60);

    // 3. Dynamic constraints with std::vector via std::span
    std::vector<Constraint> dyn;
    for (int i = 0; i < 16; ++i) {
        dyn.push_back(Constraint::fill());
    }
    auto res16 = Layout::vertical(screen, dyn);
    assert(res16.size() == 16);
    assert(res16[0].height == 2);
    assert(res16[15].height == 1);

    // 4. Exceeding 16 elements (dynamic fallback to heap)
    dyn.push_back(Constraint::fill()); // 17 elements
    auto res17 = Layout::vertical(screen, dyn);
    assert(res17.size() == 17);

    // 5. Explicit to_vector()
    std::vector<Area> vec = res16.to_vector();
    assert(vec.size() == 16);

    std::cout << "PASSED\n";
}

int main()
{
    std::cout << "========================================\n";
    std::cout << " Running koto Headless Layout & Core Tests\n";
    std::cout << "========================================\n";

    test_layout_clamping();
    test_grid_layout_and_fill();
    test_cpp23_modernizations();
    test_zero_overhead_modernizations();
    test_zero_allocation_layout();

    std::cout << "========================================\n";
    std::cout << " ALL 5 LAYOUT & CORE TESTS PASSED CLEANLY!\n";
    std::cout << "========================================\n";
    return 0;
}
