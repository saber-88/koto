#include "koto/text_input.hpp"
#include "koto/box.hpp"
#include "koto/color.hpp"
#include "koto/event.hpp"
#include "koto/frame.hpp"
#include "koto/layout.hpp"
#include "koto/text.hpp"
#include "koto/utf8.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace koto
{
    TextInputState::TextInputState(std::string_view initial_text)
        : value(initial_text), cursor(value.size())
    {}

    auto TextInputState::set_text(std::string_view text) -> void
    {
        value = std::string(text);
        cursor = value.size();
        scroll_offset = 0;
    }

    auto TextInputState::text() const -> std::string_view
    {
        return value;
    }

    auto TextInputState::clear() -> void
    {
        value.clear();
        cursor = 0;
        scroll_offset = 0;
    }

    auto TextInputState::on_key(KeyInterceptor handler) -> TextInputState&
    {
        on_key_handler = std::move(handler);
        return *this;
    }

    auto TextInputState::set_filter(InputFilter filter) -> TextInputState&
    {
        input_filter = std::move(filter);
        return *this;
    }

    auto TextInputState::insert(char32_t cp) -> bool
    {
        if (input_filter && !input_filter(value, cp))
        {
            return false;
        }

        auto encoded = encode_utf8(cp);
        if (encoded.size == 0)
        {
            return false;
        }

        cursor = std::min(cursor, value.size());
        value.insert(cursor, encoded.view());
        cursor += encoded.size;
        return true;
    }

    auto TextInputState::insert(std::string_view text) -> bool
    {
        bool changed = false;
        std::string_view remaining = text;
        while (!remaining.empty())
        {
            char32_t cp = next_codepoint(remaining);
            if (cp != 0)
            {
                changed |= insert(cp);
            }
        }
        return changed;
    }

    auto TextInputState::backspace() -> bool
    {
        if (cursor == 0 || value.empty())
        {
            return false;
        }

        cursor = std::min(cursor, value.size());
        size_t prev = cursor - 1;
        while (prev > 0 && (static_cast<uint8_t>(value[prev]) & 0xC0) == 0x80)
        {
            prev--;
        }
        value.erase(prev, cursor - prev);
        cursor = prev;
        return true;
    }

    auto TextInputState::delete_forward() -> bool
    {
        if (cursor >= value.size())
        {
            return false;
        }

        size_t next = cursor + 1;
        while (next < value.size() && (static_cast<uint8_t>(value[next]) & 0xC0) == 0x80)
        {
            next++;
        }
        value.erase(cursor, next - cursor);
        return true;
    }

    auto TextInputState::delete_word() -> bool
    {
        if (cursor == 0 || value.empty())
        {
            return false;
        }

        cursor = std::min(cursor, value.size());
        size_t pos = cursor;
        while (pos > 0 && value[pos - 1] == ' ')
        {
            pos--;
        }
        while (pos > 0 && value[pos - 1] != ' ')
        {
            pos--;
        }

        value.erase(pos, cursor - pos);
        cursor = pos;
        return true;
    }

    auto TextInputState::move_left() -> bool
    {
        if (cursor == 0)
        {
            return false;
        }

        cursor = std::min(cursor, value.size());
        size_t prev = cursor - 1;
        while (prev > 0 && (static_cast<uint8_t>(value[prev]) & 0xC0) == 0x80)
        {
            prev--;
        }
        cursor = prev;
        return true;
    }

    auto TextInputState::move_right() -> bool
    {
        if (cursor >= value.size())
        {
            return false;
        }

        size_t next = cursor + 1;
        while (next < value.size() && (static_cast<uint8_t>(value[next]) & 0xC0) == 0x80)
        {
            next++;
        }
        cursor = next;
        return true;
    }

    auto TextInputState::move_home() -> bool
    {
        if (cursor != 0)
        {
            cursor = 0;
            return true;
        }
        return false;
    }

    auto TextInputState::move_end() -> bool
    {
        if (cursor != value.size())
        {
            cursor = value.size();
            return true;
        }
        return false;
    }

    auto TextInputState::handle_event(const KeyEvent& key) -> bool
    {
        if (!focused)
        {
            return false;
        }

        // 1. Run user interceptor pre-hook if registered
        if (on_key_handler && on_key_handler(*this, key))
        {
            return true;
        }

        // 2. Navigation
        if (key.is(Key::Left))
        {
            return move_left();
        }
        if (key.is(Key::Right))
        {
            return move_right();
        }
        if (key.is(Key::Home) || (key.ctrl && (key.ch == 'A' || key.ch == 'a')))
        {
            return move_home();
        }
        if (key.is(Key::End) || (key.ctrl && (key.ch == 'E' || key.ch == 'e')))
        {
            return move_end();
        }

        // 3. Line/Word Deletion shortcuts
        if (key.ctrl && (key.ch == 'U' || key.ch == 'u'))
        {
            if (!value.empty())
            {
                clear();
                return true;
            }
            return false;
        }
        if (key.ctrl && (key.ch == 'W' || key.ch == 'w'))
        {
            return delete_word();
        }

        // 4. Character Deletion
        if (key.is(Key::Backspace))
        {
            return backspace();
        }
        if (key.is(Key::Delete))
        {
            return delete_forward();
        }

        // 5. Character Insertion (ASCII and multi-byte UTF-8)
        if (!key.ctrl && !key.alt && key.is_char())
        {
            return insert(static_cast<char32_t>(key.ch));
        }

        return false;
    }

    TextInput::TextInput(TextInputState& state)
        : m_state(state)
    {}

    TextInput& TextInput::placeholder(std::string_view placeholder)
    {
        m_placeholder = std::string(placeholder);
        return *this;
    }

    TextInput& TextInput::title(std::string_view title)
    {
        m_title = std::string(title);
        return *this;
    }

    TextInput& TextInput::border_style(BorderStyle style)
    {
        m_border_style = style;
        return *this;
    }

    TextInput& TextInput::fg(std::string_view color)
    {
        m_fg = parse_color(color);
        return *this;
    }

    TextInput& TextInput::bg(std::string_view color)
    {
        m_bg = parse_color(color);
        return *this;
    }

    TextInput& TextInput::fg(uint32_t color)
    {
        m_fg = color;
        return *this;
    }

    TextInput& TextInput::bg(uint32_t color)
    {
        m_bg = color;
        return *this;
    }

    TextInput& TextInput::placeholder_fg(std::string_view color)
    {
        m_placeholder_fg = parse_color(color);
        return *this;
    }

    TextInput& TextInput::placeholder_fg(uint32_t color)
    {
        m_placeholder_fg = color;
        return *this;
    }

    TextInput& TextInput::focused_border_fg(std::string_view color)
    {
        m_focused_border_fg = parse_color(color);
        return *this;
    }

    TextInput& TextInput::focused_border_fg(uint32_t color)
    {
        m_focused_border_fg = color;
        return *this;
    }

    void TextInput::draw(Frame& frame, Area area)
    {
        if (!area.is_valid())
        {
            return;
        }

        Area inner = area;

        // Draw border or clear inner area
        if (m_border_style != BorderStyle::None)
        {
            uint32_t border_color = m_state.focused ? m_focused_border_fg : m_fg;
            Box()
                .border_style(m_border_style)
                .fg(border_color)
                .bg(m_bg)
                .title(m_title)
                .title_fg(border_color)
                .filled(true)
                .draw(frame, area);

            inner = area.inset(1, 1, 1, 1);
        }
        else
        {
            for (int r = inner.row; r < inner.bottom(); r++)
            {
                for (int c = inner.col; c < inner.right(); c++)
                {
                    frame.set_cell(r, c, {U' ', m_fg, m_bg});
                }
            }
        }

        if (!inner.is_valid())
        {
            return;
        }

        int row = inner.row;

        // Empty text: show placeholder if available, set cursor
        if (m_state.value.empty())
        {
            if (!m_placeholder.empty())
            {
                Text(m_placeholder)
                    .fg(m_placeholder_fg)
                    .bg(m_bg)
                    .clip()
                    .draw(frame, inner);
            }

            if (m_state.focused)
            {
                frame.set_cursor(row, inner.col);
            }
            return;
        }

        // Measure visual character positions and determine cursor column
        m_state.cursor = std::min(m_state.cursor, m_state.value.size());

        struct VisualChar
        {
            char32_t cp = 0;
            int col_start = 0;
            int width = 0;
        };

        std::vector<VisualChar> chars;
        int cur_col = 0;
        int cursor_visual_col = 0;

        std::string_view remaining = m_state.value;
        while (!remaining.empty())
        {
            size_t byte_idx = static_cast<size_t>(remaining.data() - m_state.value.data());
            if (byte_idx == m_state.cursor)
            {
                cursor_visual_col = cur_col;
            }
            char32_t cp = next_codepoint(remaining);
            int w = codepoint_width(cp);
            chars.push_back({cp, cur_col, w});
            cur_col += w;
        }

        if (m_state.cursor >= m_state.value.size())
        {
            cursor_visual_col = cur_col;
        }

        // Horizontal scroll adjustment
        if (cursor_visual_col < m_state.scroll_offset)
        {
            m_state.scroll_offset = cursor_visual_col;
        }
        if (cursor_visual_col >= m_state.scroll_offset + inner.width)
        {
            m_state.scroll_offset = cursor_visual_col - inner.width + 1;
        }
        if (m_state.scroll_offset < 0)
        {
            m_state.scroll_offset = 0;
        }

        // Draw visible characters
        for (const auto& ch : chars)
        {
            int screen_col = inner.col + (ch.col_start - m_state.scroll_offset);
            if (screen_col >= inner.col && screen_col + ch.width <= inner.right())
            {
                frame.set_cell(row, screen_col, {ch.cp, m_fg, m_bg});
            }
        }

        // Position hardware cursor
        if (m_state.focused)
        {
            int cursor_screen_col = inner.col + (cursor_visual_col - m_state.scroll_offset);
            if (cursor_screen_col >= inner.col && cursor_screen_col < inner.right())
            {
                frame.set_cursor(row, cursor_screen_col);
            }
        }
    }
}
