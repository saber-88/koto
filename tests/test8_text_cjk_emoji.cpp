#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <koto/koto.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace koto;

void test_utf8_encoding_decoding()
{
    std::cout << "[Test 1] UTF-8 encoding and decoding round-trip... ";

    // 1-byte ASCII ('A')
    const char* s1 = "A";
    const char* p1 = s1;
    char32_t cp1 = next_codepoint(p1, s1 + 1);
    assert(cp1 == U'A');
    assert(p1 == s1 + 1);

    char buf1[5] = {};
    int len1 = encode_utf8(cp1, buf1);
    assert(len1 == 1);
    assert(buf1[0] == 'A');

    // 2-byte Latin ('é' = U+00E9)
    const char* s2 = "é";
    const char* p2 = s2;
    char32_t cp2 = next_codepoint(p2, s2 + 2);
    assert(cp2 == U'é');
    assert(p2 == s2 + 2);

    char buf2[5] = {};
    int len2 = encode_utf8(cp2, buf2);
    assert(len2 == 2);
    assert(std::string(buf2, 2) == "é");

    // 3-byte CJK ('琴' = U+7434)
    const char* s3 = "琴";
    const char* p3 = s3;
    char32_t cp3 = next_codepoint(p3, s3 + 3);
    assert(cp3 == U'琴');
    assert(p3 == s3 + 3);

    char buf3[5] = {};
    int len3 = encode_utf8(cp3, buf3);
    assert(len3 == 3);
    assert(std::string(buf3, 3) == "琴");

    // 4-byte Emoji ('🚀' = U+1F680)
    std::string s4 = "🚀";
    const char* p4 = s4.data();
    char32_t cp4 = next_codepoint(p4, p4 + s4.size());
    assert(cp4 == U'🚀');
    assert(p4 == s4.data() + s4.size());

    char buf4[5] = {};
    int len4 = encode_utf8(cp4, buf4);
    assert(len4 == 4);
    assert(std::string(buf4, 4) == "🚀");

    std::cout << "PASSED\n";
}

void test_codepoint_widths()
{
    std::cout << "[Test 2] Codepoint & text visual widths... ";

    // Standard ASCII
    assert(codepoint_width(U'A') == 1);
    assert(codepoint_width(U'1') == 1);
    assert(codepoint_width(U' ') == 1);

    // CJK ideographs (Kanji, Hanzi, Hangul) are 2 columns wide
    assert(codepoint_width(U'琴') == 2);
    assert(codepoint_width(U'中') == 2);
    assert(codepoint_width(U'文') == 2);
    assert(codepoint_width(U'日') == 2);
    assert(codepoint_width(U'本') == 2);
    assert(codepoint_width(U'語') == 2);
    assert(codepoint_width(U'한') == 2);
    assert(codepoint_width(U'글') == 2);

    // Emojis are 2 columns wide
    assert(codepoint_width(U'🚀') == 2);
    assert(codepoint_width(U'🦀') == 2);
    assert(codepoint_width(U'🎉') == 2);
    assert(codepoint_width(U'🔥') == 2);
    assert(codepoint_width(U'⭐') == 2);

    // Fullwidth forms
    assert(codepoint_width(U'Ａ') == 2); // Fullwidth Latin A
    assert(codepoint_width(U'１') == 2); // Fullwidth Digit 1

    // Zero-width characters
    assert(codepoint_width(U'\0') == 0);
    assert(codepoint_width(0x200B) == 0); // Zero-width space
    assert(codepoint_width(0x200D) == 0); // Zero-width joiner
    assert(codepoint_width(0xFE0F) == 0); // Emoji variation selector-16

    // Compound text visual width
    // "Hello 琴" -> 5 + 1 + 2 = 8
    assert(text_width("Hello 琴") == 8);

    // "🚀 Koto 琴" -> 2 + 1 + 4 + 1 + 2 = 10
    assert(text_width("🚀 Koto 琴") == 10);

    // "東京 (Tokyo)" -> 2 (東) + 2 (京) + 1 (space) + 7 ((Tokyo)) = 12
    assert(text_width("東京 (Tokyo)") == 12);

    // Emojis with Variation Selector 16 and Zero-Width-Joiner (ZWJ) composite sequences
    assert(text_width("❤️") == 2);
    assert(text_width("👨‍👩‍👧‍👦") == 2);
    assert(text_width("👩‍💻") == 2);
    assert(text_width("🏳️‍🌈") == 2);

    // Standard text symbols in Dingbats and Misc Symbols default to width 1 (NOT wide emojis)
    assert(codepoint_width(U'★') == 1);
    assert(codepoint_width(U'☆') == 1);
    assert(codepoint_width(U'✓') == 1);
    assert(codepoint_width(U'✔') == 1);
    assert(codepoint_width(U'✕') == 1);
    assert(codepoint_width(U'✖') == 1);
    assert(codepoint_width(U'➔') == 1);

    // Standard TR51 BMP emojis default to width 2
    assert(codepoint_width(0x2B50) == 2); // ⭐
    assert(codepoint_width(0x26A1) == 2); // ⚡
    assert(codepoint_width(0x2728) == 2); // ✨

    // Text-presentation symbols vs emoji-presentation (with VS16)
    assert(text_width("🗑") == 1);
    assert(text_width("🗑️") == 2);
    assert(text_width("ℹ") == 1);
    assert(text_width("ℹ️") == 2);
    assert(text_width("⚠️") == 2);
    assert(text_width("⚙️") == 2);

    // Advanced Emoji Grapheme Clusters
    assert(text_width("1️⃣") == 2); // Keycap sequence
    assert(text_width("🇺🇸") == 2); // Regional indicator flag pair
    assert(text_width("👍🏽") == 2); // Skin tone modifier

    std::cout << "PASSED\n";
}

void test_frame_continuation_cells()
{
    std::cout << "[Test 3] Frame 2-column continuation cells & overwrite safety... ";
    Frame frame(3, 10);

    // Write wide character '琴' at row 0, col 0
    frame.set_cell(0, 0, {U'琴', Color::Cyan, Color::Default});

    // Col 0 should hold '琴' with width 2, not a continuation
    Cell c0 = frame.get_cell(0, 0);
    assert(c0.ch == U'琴');
    assert(c0.width == 2);
    assert(!c0.continuation);

    // Col 1 must automatically be marked as continuation cell
    Cell c1 = frame.get_cell(0, 1);
    assert(c1.continuation);

    // Plain text row must produce "琴" without an extra duplicate space
    assert(frame.to_plain_text_row(0) == "琴");

    // Overwrite safety: write 'X' directly into col 1 (the continuation cell)
    frame.set_cell(0, 1, {U'X', Color::White, Color::Default});

    // The owner at col 0 must safely revert to ' ' with width 1
    Cell owner = frame.get_cell(0, 0);
    assert(owner.ch == U' ');
    assert(owner.width == 1);

    // Col 1 now holds 'X'
    Cell overwritten = frame.get_cell(0, 1);
    assert(overwritten.ch == U'X');
    assert(!overwritten.continuation);

    // Edge safety: write wide char '🚀' at the very last column (col = 9 in a 10-col frame)
    frame.set_cell(0, 9, {U'🚀', Color::Yellow, Color::Default});
    // Since there is no room for a 2-column wide character at the last cell, it is replaced with a single space
    Cell edge = frame.get_cell(0, 9);
    assert(edge.ch == U' ');
    assert(edge.width == 1);

    std::cout << "PASSED\n";
}

void test_text_widget_cjk_and_emojis()
{
    std::cout << "[Test 4] Text widget rendering CJK & emojis... ";
    Frame frame(4, 25);

    Text("琴 (koto) - C++23 TUI 🚀")
        .fg(Color::Cyan)
        .draw(frame, frame.area());

    std::string row0 = frame.to_plain_text_row(0);
    assert(row0 == "琴 (koto) - C++23 TUI 🚀");

    // Multi-line CJK text
    frame.clear_back_buffer();
    Text("日本語\n中文\n한국어\nEnglish 🚀")
        .draw(frame, frame.area());

    assert(frame.to_plain_text_row(0) == "日本語");
    assert(frame.to_plain_text_row(1) == "中文");
    assert(frame.to_plain_text_row(2) == "한국어");
    assert(frame.to_plain_text_row(3) == "English 🚀");

    std::cout << "PASSED\n";
}

void test_text_alignment_with_wide_characters()
{
    std::cout << "[Test 5] Text alignment with CJK & emojis... ";
    Frame frame(3, 20);

    // Content: "🚀 琴 🚀"
    // Width: 2 (🚀) + 1 (space) + 2 (琴) + 1 (space) + 2 (🚀) = 8 columns
    // In width 20:
    // Center: (20 - 8) / 2 = 6 spaces before
    Text("🚀 琴 🚀")
        .align(TextAlign::Center)
        .draw(frame, frame.area());

    // The first cell should be space at (0, 0)
    assert(frame.get_cell(0, 0).ch == U' ');
    // The text starts at col 6: col 6 holds '🚀'
    assert(frame.get_cell(0, 6).ch == U'🚀');
    assert(frame.get_cell(0, 7).continuation);
    assert(frame.get_cell(0, 8).ch == U' ');
    assert(frame.get_cell(0, 9).ch == U'琴');
    assert(frame.get_cell(0, 10).continuation);

    // Right-aligned in width 20: 20 - 8 = 12 spaces before
    frame.clear_back_buffer();
    Text("🚀 琴 🚀")
        .align(TextAlign::Right)
        .draw(frame, frame.area());

    // Should start at column 12
    assert(frame.get_cell(0, 12).ch == U'🚀');
    assert(frame.get_cell(0, 13).continuation);

    std::cout << "PASSED\n";
}

void test_text_overflow_with_cjk()
{
    std::cout << "[Test 6] Text overflow and clipping with CJK... ";
    Frame frame(2, 10);

    // "日本語プログラミング" (Japanese programming) has 10 characters = 20 visual columns!
    // Inside width 10 with Ellipsis: should display as much as fits with '…' at the end
    Text("日本語プログラミング")
        .overflow(TextOverflow::Ellipsis)
        .draw(frame, frame.area());

    std::string row = frame.to_plain_text_row(0);
    assert(row.find("…") != std::string::npos);

    // Inside width 10 with Clip: clips cleanly
    frame.clear_back_buffer();
    Text("日本語プログラミング")
        .overflow(TextOverflow::Clip)
        .draw(frame, frame.area());

    std::string clipped = frame.to_plain_text_row(0);
    assert(clipped.find("…") == std::string::npos);
    assert(clipped.rfind("日本語", 0) == 0);

    std::cout << "PASSED\n";
}

void test_box_and_list_with_cjk_and_emojis()
{
    std::cout << "[Test 7] Box titles & List items with CJK & emojis... ";
    Frame frame(5, 25);

    // Box with CJK and emoji title
    Box()
        .border_style(BorderStyle::Rounded)
        .title(" 琴 Koto 🚀 ")
        .title_align(TitleAlign::Center)
        .draw(frame, frame.area());

    std::string top_row = frame.to_plain_text_row(0);
    assert(top_row.find("琴 Koto 🚀") != std::string::npos);
    // Border should close cleanly with '╮'
    assert(top_row.find("╮") != std::string::npos);

    // List with CJK items and emoji highlight symbol
    frame.clear_back_buffer();
    ListState state({"東京 (Tokyo)", "北京 (Beijing)", "Seoul (서울)"});
    state.selected = 0;

    List(state)
        .highlight_symbol("👉 ")
        .draw(frame, frame.area());

    assert(frame.to_plain_text_row(0) == "👉 東京 (Tokyo)");
    // Unselected items should be padded to match the 2-column symbol width
    assert(frame.to_plain_text_row(1) == "   北京 (Beijing)");

    std::cout << "PASSED\n";
}

void test_cell_grapheme_clusters()
{
    std::cout << "[Test 8] Cell owning multi-codepoint grapheme clusters... ";
    Frame frame(2, 20);

    // 1. Emoji with Variation Selector: ❤️ (U+2764, U+FE0F - 2 codepoints, 6 bytes, width 2)
    Text("❤️ Heart").draw(frame, frame.area());
    Cell c_heart = frame.get_cell(0, 0);
    assert(c_heart.view() == "❤️");
    assert(c_heart.width == 2);
    assert(frame.get_cell(0, 1).continuation);
    assert(frame.to_plain_text_row(0) == "❤️ Heart");

    // 2. Flag Sequence: 🇺🇸 (U+1F1FA, U+1F1F8 - 2 codepoints, 8 bytes, width 2)
    frame.clear_back_buffer();
    Text("🇺🇸 USA").draw(frame, frame.area());
    Cell c_flag = frame.get_cell(0, 0);
    assert(c_flag.view() == "🇺🇸");
    assert(c_flag.width == 2);
    assert(frame.get_cell(0, 1).continuation);
    assert(frame.to_plain_text_row(0) == "🇺🇸 USA");

    // 3. Skin-tone Modifier: 👍🏽 (U+1F44D, U+1F3FD - 2 codepoints, 8 bytes, width 2)
    frame.clear_back_buffer();
    Text("👍🏽 Good").draw(frame, frame.area());
    Cell c_skin = frame.get_cell(0, 0);
    assert(c_skin.view() == "👍🏽");
    assert(c_skin.width == 2);
    assert(frame.get_cell(0, 1).continuation);
    assert(frame.to_plain_text_row(0) == "👍🏽 Good");

    // 4. Keycap Sequence: 1️⃣ ('1', U+FE0F, U+20E3 - 3 codepoints, 7 bytes, width 2)
    frame.clear_back_buffer();
    Text("1️⃣ One").draw(frame, frame.area());
    Cell c_keycap = frame.get_cell(0, 0);
    assert(c_keycap.view() == "1️⃣");
    assert(c_keycap.width == 2);
    assert(frame.get_cell(0, 1).continuation);
    assert(frame.to_plain_text_row(0) == "1️⃣ One");

    // 5. ZWJ Sequence: 👩‍💻 (U+1F469, U+200D, U+1F4BB - 3 codepoints, 11 bytes, width 2)
    frame.clear_back_buffer();
    Text("👩‍💻 Dev").draw(frame, frame.area());
    Cell c_zwj = frame.get_cell(0, 0);
    assert(c_zwj.view() == "👩‍💻");
    assert(c_zwj.width == 2);
    assert(frame.get_cell(0, 1).continuation);
    assert(frame.to_plain_text_row(0) == "👩‍💻 Dev");

    // 6. Complex 25-byte ZWJ Family Emoji: 👨‍👩‍👧‍👦 (4 personas + 3 ZWJs = 25 bytes, width 2)
    frame.clear_back_buffer();
    Text("👨‍👩‍👧‍👦 Family").draw(frame, frame.area());
    Cell c_family = frame.get_cell(0, 0);
    assert(c_family.view() == "👨‍👩‍👧‍👦");
    assert(c_family.grapheme.size == 25);
    assert(c_family.width == 2);
    assert(frame.get_cell(0, 1).continuation);
    assert(frame.to_plain_text_row(0) == "👨‍👩‍👧‍👦 Family");

    // Static assertion verifying Cell cache-line alignment
    static_assert(sizeof(Cell) == 64, "Cell must be exactly 64 bytes (1 cache line)");

    std::cout << "PASSED\n";
}

void test_grapheme_editing_and_sticky_cursor()
{
    std::cout << "[Test 9] Grapheme-atomic backspace, delete, and sticky cursor... ";

    // 1. TextInputState atomic backspace & delete
    TextInputState input;
    input.insert("❤️");
    assert(input.value == "❤️");
    assert(input.cursor == 6); // 6 bytes: \xE2\x9D\xA4\xEF\xB8\x8F

    // Move left should skip the whole grapheme in one step
    input.move_left();
    assert(input.cursor == 0);
    input.move_right();
    assert(input.cursor == 6);

    // Backspace should erase the whole ❤️ (not leaving orphan \xE2\x9D\xA4)
    input.backspace();
    assert(input.value.empty());
    assert(input.cursor == 0);

    // Atomic flag deletion
    input.insert("🇺🇸");
    assert(input.value == "🇺🇸");
    assert(input.cursor == 8);
    input.backspace();
    assert(input.value.empty());

    // 2. TextAreaState sticky cursor across CJK and ASCII lines
    TextAreaState editor;
    editor.text = {"こんにちは", "0123456789"};
    // Line 0 has 5 CJK characters = 10 visual columns (15 bytes)
    // Line 1 has 10 ASCII characters = 10 visual columns (10 bytes)

    editor.cursor_row = 0;
    editor.move_end();
    assert(editor.cursor_row == 0);
    assert(editor.cursor_col == 15);
    assert(editor.preferred_col == 10); // Visual column 10!

    // Move down to Line 1: should land at visual column 10 (byte 10)
    editor.move_down();
    assert(editor.cursor_row == 1);
    assert(editor.cursor_col == 10);
    assert(editor.preferred_col == 10);

    // Move up to Line 0: should land back at visual column 10 (byte 15) with ZERO drift!
    editor.move_up();
    assert(editor.cursor_row == 0);
    assert(editor.cursor_col == 15);
    assert(editor.preferred_col == 10);

    // 3. TextAreaState atomic grapheme deletion
    editor.clear();
    editor.insert("👩‍💻");
    assert(editor.text[0] == "👩‍💻");
    assert(editor.cursor_col == 11);
    editor.backspace();
    assert(editor.text[0].empty());
    assert(editor.cursor_col == 0);

    std::cout << "PASSED\n";
}

int main()
{
    std::cout << "========================================\n";
    std::cout << " Running koto CJK & Emoji Test Suite\n";
    std::cout << "========================================\n";

    test_utf8_encoding_decoding();
    test_codepoint_widths();
    test_frame_continuation_cells();
    test_text_widget_cjk_and_emojis();
    test_text_alignment_with_wide_characters();
    test_text_overflow_with_cjk();
    test_box_and_list_with_cjk_and_emojis();
    test_cell_grapheme_clusters();
    test_grapheme_editing_and_sticky_cursor();

    std::cout << "========================================\n";
    std::cout << " ALL 9 CJK & EMOJI TESTS PASSED CLEANLY!\n";
    std::cout << "========================================\n";
    return 0;
}
