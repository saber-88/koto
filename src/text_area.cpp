#include "koto/text_area.hpp"
#include "koto/box.hpp"
#include "koto/color.hpp"
#include "koto/frame.hpp"
#include "koto/utf8.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace koto
{
    namespace
    {
        // Adjusts col backwards if it lands in the middle of a multi-byte UTF-8 sequence
        void align_cursor_to_utf8(const std::string& line, size_t& col)
        {
            if (col > line.size())
            {
                col = line.size();
            }
            while (col > 0 && (static_cast<uint8_t>(line[col]) & 0xC0) == 0x80)
            {
                col--;
            }
        }

        // Computes the visual terminal column corresponding to byte_pos in line
        int compute_visual_col(const std::string& line, size_t byte_pos)
        {
            int visual_col = 0;
            const char* ptr = line.data();
            const char* const end = ptr + std::min(byte_pos, line.size());

            while (ptr < end)
            {
                char32_t cp = next_codepoint(ptr, end);
                visual_col += codepoint_width(cp);
            }
            return visual_col;
        }

        void clamp_state(TextAreaState& state)
        {
            if (state.text.empty())
            {
                state.text.push_back("");
            }
            if (state.cursor_row >= state.text.size())
            {
                state.cursor_row = state.text.size() - 1;
            }
            align_cursor_to_utf8(state.text[state.cursor_row], state.cursor_col);
        }
    }

    // =========================================================================
    // TextAreaState Implementation
    // =========================================================================

    TextAreaState::TextAreaState()
        : text{""}, cursor_row(0), cursor_col(0), preferred_col(0),
          scroll_row(0), scroll_col(0), scroll_margin(1),
          show_line_number(true), cursor_mode(CursorColumnMode::Sticky),
          scroll_mode(ScrollMode::Margin)
    {}

    TextAreaState::TextAreaState(std::string_view initial_text)
        : cursor_row(0), cursor_col(0), preferred_col(0),
          scroll_row(0), scroll_col(0), scroll_margin(1),
          show_line_number(true), cursor_mode(CursorColumnMode::Sticky),
          scroll_mode(ScrollMode::Margin)
    {
        set_text(initial_text);
    }

    TextAreaState::TextAreaState(std::vector<std::string> initial_lines)
        : text(std::move(initial_lines)), cursor_row(0), cursor_col(0), preferred_col(0),
          scroll_row(0), scroll_col(0), scroll_margin(1),
          show_line_number(true), cursor_mode(CursorColumnMode::Sticky),
          scroll_mode(ScrollMode::Margin)
    {
        if (text.empty())
        {
            text.push_back("");
        }
    }

    void TextAreaState::insert(char32_t cp)
    {
        clamp_state(*this);

        if (cp == U'\n')
        {
            newline();
            return;
        }
        if (cp == U'\r')
        {
            return;
        }

        char buf[5] = {};
        int len = encode_utf8(cp, buf);
        if (len <= 0)
        {
            return;
        }

        auto& line = text[cursor_row];
        align_cursor_to_utf8(line, cursor_col);
        line.insert(cursor_col, buf, static_cast<size_t>(len));
        cursor_col += static_cast<size_t>(len);
        preferred_col = cursor_col;
    }

    void TextAreaState::insert(std::string_view str)
    {
        const char* ptr = str.data();
        const char* const end = ptr + str.size();
        while (ptr < end)
        {
            char32_t cp = next_codepoint(ptr, end);
            if (cp != 0)
            {
                insert(cp);
            }
        }
    }

    void TextAreaState::newline()
    {
        clamp_state(*this);
        auto& line = text[cursor_row];
        align_cursor_to_utf8(line, cursor_col);

        std::string right_part = line.substr(cursor_col);
        line.erase(cursor_col);
        text.insert(text.begin() + static_cast<std::ptrdiff_t>(cursor_row + 1), std::move(right_part));

        cursor_row++;
        cursor_col = 0;
        preferred_col = 0;
    }

    void TextAreaState::backspace()
    {
        clamp_state(*this);
        auto& line = text[cursor_row];

        if (cursor_col > 0)
        {
            if (cursor_col > line.size())
            {
                cursor_col = line.size();
            }
            size_t prev = cursor_col - 1;
            while (prev > 0 && (static_cast<uint8_t>(line[prev]) & 0xC0) == 0x80)
            {
                prev--;
            }
            line.erase(prev, cursor_col - prev);
            cursor_col = prev;
            preferred_col = cursor_col;
        }
        else if (cursor_col == 0 && cursor_row > 0)
        {
            size_t prev_len = text[cursor_row - 1].size();
            text[cursor_row - 1].append(line);
            text.erase(text.begin() + static_cast<std::ptrdiff_t>(cursor_row));
            cursor_row--;
            cursor_col = prev_len;
            preferred_col = cursor_col;
        }
    }

    void TextAreaState::delete_forward()
    {
        clamp_state(*this);
        auto& line = text[cursor_row];

        if (cursor_col < line.size())
        {
            size_t next = cursor_col + 1;
            while (next < line.size() && (static_cast<uint8_t>(line[next]) & 0xC0) == 0x80)
            {
                next++;
            }
            line.erase(cursor_col, next - cursor_col);
        }
        else if (cursor_col >= line.size() && cursor_row + 1 < text.size())
        {
            line.append(text[cursor_row + 1]);
            text.erase(text.begin() + static_cast<std::ptrdiff_t>(cursor_row + 1));
        }
    }

    void TextAreaState::move_left()
    {
        clamp_state(*this);
        if (cursor_col > 0)
        {
            auto& line = text[cursor_row];
            if (cursor_col > line.size())
            {
                cursor_col = line.size();
            }
            size_t prev = cursor_col - 1;
            while (prev > 0 && (static_cast<uint8_t>(line[prev]) & 0xC0) == 0x80)
            {
                prev--;
            }
            cursor_col = prev;
            preferred_col = cursor_col;
        }
        else if (cursor_col == 0 && cursor_row > 0)
        {
            cursor_row--;
            cursor_col = text[cursor_row].size();
            preferred_col = cursor_col;
        }
    }

    void TextAreaState::move_right()
    {
        clamp_state(*this);
        auto& line = text[cursor_row];
        if (cursor_col < line.size())
        {
            size_t next = cursor_col + 1;
            while (next < line.size() && (static_cast<uint8_t>(line[next]) & 0xC0) == 0x80)
            {
                next++;
            }
            cursor_col = next;
            preferred_col = cursor_col;
        }
        else if (cursor_col >= line.size() && cursor_row + 1 < text.size())
        {
            cursor_row++;
            cursor_col = 0;
            preferred_col = cursor_col;
        }
    }

    void TextAreaState::move_up()
    {
        clamp_state(*this);
        if (cursor_row > 0)
        {
            cursor_row--;
            if (cursor_mode == CursorColumnMode::Sticky)
            {
                cursor_col = std::min(preferred_col, text[cursor_row].size());
            }
            else
            {
                cursor_col = std::min(cursor_col, text[cursor_row].size());
                preferred_col = cursor_col;
            }
            align_cursor_to_utf8(text[cursor_row], cursor_col);
        }
    }

    void TextAreaState::move_down()
    {
        clamp_state(*this);
        if (cursor_row + 1 < text.size())
        {
            cursor_row++;
            if (cursor_mode == CursorColumnMode::Sticky)
            {
                cursor_col = std::min(preferred_col, text[cursor_row].size());
            }
            else
            {
                cursor_col = std::min(cursor_col, text[cursor_row].size());
                preferred_col = cursor_col;
            }
            align_cursor_to_utf8(text[cursor_row], cursor_col);
        }
    }

    void TextAreaState::move_home()
    {
        cursor_col = 0;
        preferred_col = 0;
    }

    void TextAreaState::move_end()
    {
        clamp_state(*this);
        cursor_col = text[cursor_row].size();
        preferred_col = cursor_col;
    }

    void TextAreaState::clear()
    {
        text.clear();
        text.push_back("");
        cursor_row = 0;
        cursor_col = 0;
        preferred_col = 0;
        scroll_row = 0;
        scroll_col = 0;
    }

    void TextAreaState::set_text(std::string_view new_text)
    {
        text.clear();
        size_t start = 0;
        while (start < new_text.size())
        {
            size_t end = new_text.find('\n', start);
            if (end == std::string_view::npos)
            {
                std::string line(new_text.substr(start));
                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }
                text.push_back(std::move(line));
                break;
            }
            std::string line(new_text.substr(start, end - start));
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            text.push_back(std::move(line));
            start = end + 1;
        }

        if (text.empty())
        {
            text.push_back("");
        }

        cursor_row = 0;
        cursor_col = 0;
        preferred_col = 0;
        scroll_row = 0;
        scroll_col = 0;
    }

    std::string TextAreaState::to_string() const
    {
        std::string result;
        for (auto [i, line] : std::views::enumerate(text))
        {
            result.append(line);
            if (static_cast<size_t>(i) + 1 < text.size())
            {
                result.push_back('\n');
            }
        }
        return result;
    }

    bool TextAreaState::handle_event(const KeyEvent& key)
    {
        if (key.is(Key::Up))
        {
            move_up();
            return true;
        }
        if (key.is(Key::Down))
        {
            move_down();
            return true;
        }
        if (key.is(Key::Left))
        {
            move_left();
            return true;
        }
        if (key.is(Key::Right))
        {
            move_right();
            return true;
        }
        if (key.is(Key::Home) || (key.ctrl && (key.ch == 'A' || key.ch == 'a')))
        {
            move_home();
            return true;
        }
        if (key.is(Key::End) || (key.ctrl && (key.ch == 'E' || key.ch == 'e')))
        {
            move_end();
            return true;
        }
        if (key.is(Key::Backspace))
        {
            backspace();
            return true;
        }
        if (key.is(Key::Delete))
        {
            delete_forward();
            return true;
        }
        if (key.is(Key::Enter))
        {
            newline();
            return true;
        }
        if (key.is(Key::Tab))
        {
            insert("    ");
            return true;
        }
        if (!key.ctrl && !key.alt && key.is_char())
        {
            insert(static_cast<char32_t>(key.ch));
            return true;
        }

        return false;
    }

    // =========================================================================
    // TextArea Widget Implementation
    // =========================================================================

    TextArea::TextArea(TextAreaState& state)
        : m_state(state)
    {}

    TextArea& TextArea::border_style(BorderStyle style)
    {
        m_border_style = style;
        return *this;
    }

    TextArea& TextArea::title(std::string_view title)
    {
        m_title = std::string(title);
        return *this;
    }

    TextArea& TextArea::title_align(TitleAlign align)
    {
        m_title_align = align;
        return *this;
    }

    TextArea& TextArea::line_numbers(bool show)
    {
        m_show_line_numbers = show;
        return *this;
    }

    TextArea& TextArea::fg(std::string_view fg)
    {
        m_fg = parse_color(fg);
        return *this;
    }

    TextArea& TextArea::fg(uint32_t fg)
    {
        m_fg = fg;
        return *this;
    }

    TextArea& TextArea::bg(std::string_view bg)
    {
        m_bg = parse_color(bg);
        return *this;
    }

    TextArea& TextArea::bg(uint32_t bg)
    {
        m_bg = bg;
        return *this;
    }

    TextArea& TextArea::line_number_fg(std::string_view fg)
    {
        m_line_number_fg = parse_color(fg);
        return *this;
    }

    TextArea& TextArea::line_number_fg(uint32_t fg)
    {
        m_line_number_fg = fg;
        return *this;
    }

    TextArea& TextArea::focused(bool f)
    {
        m_focused = f;
        return *this;
    }

    TextArea& TextArea::cursor_mode(CursorColumnMode mode)
    {
        m_state.cursor_mode = mode;
        return *this;
    }

    TextArea& TextArea::scroll_mode(ScrollMode mode, size_t margin)
    {
        m_state.scroll_mode = mode;
        m_state.scroll_margin = margin;
        return *this;
    }

    void TextArea::draw(Frame& frame, Area area)
    {
        if (!area.is_valid())
        {
            return;
        }

        Area inner = area;

        if (m_border_style != BorderStyle::None)
        {
            uint32_t border_col = m_focused ? Color::Cyan : m_fg;
            Box()
                .border_style(m_border_style)
                .title(m_title)
                .title_align(m_title_align)
                .fg(border_col)
                .bg(m_bg)
                .title_fg(border_col)
                .filled(true)
                .draw(frame, area);

            inner = area.inset(1, 1, 1, 1);
        }
        else
        {
            for (int r = inner.row; r < inner.bottom(); ++r)
            {
                for (int c = inner.col; c < inner.right(); ++c)
                {
                    frame.set_cell(r, c, {U' ', m_fg, m_bg});
                }
            }
        }

        if (!inner.is_valid() || inner.width <= 0 || inner.height <= 0)
        {
            return;
        }

        clamp_state(m_state);

        // Gutter calculation
        bool show_gutter = m_show_line_numbers && m_state.show_line_number;
        int gutter_w = 0;
        int digit_w = 0;

        if (show_gutter)
        {
            size_t total_lines = m_state.text.size();
            digit_w = static_cast<int>(std::to_string(std::max<size_t>(1, total_lines)).size());
            digit_w = std::max(2, digit_w);
            gutter_w = digit_w + 2; // e.g. "  1 │" (digit_w chars + space + bar)
            if (inner.width <= gutter_w + 1)
            {
                show_gutter = false;
                gutter_w = 0;
            }
        }

        Area content_area(inner.row, inner.col + gutter_w, inner.width - gutter_w, inner.height);
        if (!content_area.is_valid() || content_area.width <= 0)
        {
            return;
        }

        // Compute cursor visual column
        int cursor_visual_col = compute_visual_col(m_state.text[m_state.cursor_row], m_state.cursor_col);

        // Viewport scroll adjustment
        const size_t view_h = static_cast<size_t>(content_area.height);
        const size_t view_w = static_cast<size_t>(content_area.width);
        const size_t v_margin = (m_state.scroll_mode == ScrollMode::Margin)
            ? std::min(m_state.scroll_margin, view_h / 2)
            : 0;
        const size_t h_margin = (m_state.scroll_mode == ScrollMode::Margin)
            ? std::min(m_state.scroll_margin * 2, view_w / 2)
            : 0;

        // Vertical scroll
        if (m_state.cursor_row < m_state.scroll_row + v_margin)
        {
            m_state.scroll_row = (m_state.cursor_row >= v_margin) ? m_state.cursor_row - v_margin : 0;
        }
        if (m_state.cursor_row + v_margin >= m_state.scroll_row + view_h)
        {
            if (m_state.cursor_row + v_margin + 1 >= view_h)
            {
                m_state.scroll_row = m_state.cursor_row + v_margin + 1 - view_h;
            }
            else
            {
                m_state.scroll_row = 0;
            }
        }

        // Horizontal scroll
        const size_t cur_vis = static_cast<size_t>(std::max(0, cursor_visual_col));
        if (cur_vis < m_state.scroll_col + h_margin)
        {
            m_state.scroll_col = (cur_vis >= h_margin) ? cur_vis - h_margin : 0;
        }
        if (cur_vis + h_margin >= m_state.scroll_col + view_w)
        {
            if (cur_vis + h_margin + 1 >= view_w)
            {
                m_state.scroll_col = cur_vis + h_margin + 1 - view_w;
            }
            else
            {
                m_state.scroll_col = 0;
            }
        }

        // Render visible lines and gutter
        for (int r = 0; r < content_area.height; ++r)
        {
            int screen_row = content_area.row + r;
            size_t line_idx = m_state.scroll_row + static_cast<size_t>(r);

            // Draw Gutter
            if (show_gutter)
            {
                if (line_idx < m_state.text.size())
                {
                    std::string num_str = std::to_string(line_idx + 1);
                    int pad = digit_w - static_cast<int>(num_str.size());
                    int g_col = inner.col;

                    for (int p = 0; p < pad; ++p)
                    {
                        frame.set_cell(screen_row, g_col++, {U' ', m_line_number_fg, m_bg});
                    }
                    for (char ch : num_str)
                    {
                        frame.set_cell(screen_row, g_col++, {static_cast<char32_t>(ch), m_line_number_fg, m_bg});
                    }
                    frame.set_cell(screen_row, g_col++, {U' ', m_line_number_fg, m_bg});
                    frame.set_cell(screen_row, g_col, {U'│', m_line_number_fg, m_bg});
                }
                else
                {
                    int g_col = inner.col;
                    for (int w = 0; w < gutter_w - 1; ++w)
                    {
                        frame.set_cell(screen_row, g_col++, {U' ', m_line_number_fg, m_bg});
                    }
                    frame.set_cell(screen_row, g_col, {U'│', m_line_number_fg, m_bg});
                }
            }

            // Draw line content
            if (line_idx < m_state.text.size())
            {
                const auto& line = m_state.text[line_idx];
                const char* ptr = line.data();
                const char* const end = ptr + line.size();
                int cur_x = 0;

                while (ptr < end)
                {
                    char32_t cp = next_codepoint(ptr, end);
                    int w = codepoint_width(cp);
                    int rel_col = cur_x - static_cast<int>(m_state.scroll_col);

                    if (rel_col >= 0 && rel_col + w <= content_area.width)
                    {
                        frame.set_cell(screen_row, content_area.col + rel_col, {cp, m_fg, m_bg});
                    }
                    cur_x += w;
                }
            }
        }

        // Set hardware cursor if focused
        if (m_focused)
        {
            int cursor_rel_row = static_cast<int>(m_state.cursor_row) - static_cast<int>(m_state.scroll_row);
            int cursor_rel_col = cursor_visual_col - static_cast<int>(m_state.scroll_col);

            if (cursor_rel_row >= 0 && cursor_rel_row < content_area.height &&
                cursor_rel_col >= 0 && cursor_rel_col < content_area.width)
            {
                frame.set_cursor(content_area.row + cursor_rel_row, content_area.col + cursor_rel_col);
            }
        }
    }
}
