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

    std::cout << "========================================\n";
    std::cout << " ALL 6 HEADLESS TESTS PASSED CLEANLY!\n";
    std::cout << "========================================\n";
    return 0;
}
