#include "koto/tabs.hpp"
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
    // =========================================================================
    // TabsState Implementation
    // =========================================================================

    TabsState::TabsState(std::vector<std::string> initial_titles, size_t initial_selected)
        : titles(std::move(initial_titles)), selected(initial_selected)
    {
        if (!titles.empty() && selected >= titles.size())
        {
            selected = titles.size() - 1;
        }
    }

    void TabsState::select_next()
    {
        if (titles.empty())
        {
            return;
        }
        selected = (selected + 1) % titles.size();
        if (on_change)
        {
            on_change(selected, titles[selected]);
        }
    }

    void TabsState::select_prev()
    {
        if (titles.empty())
        {
            return;
        }
        selected = (selected + titles.size() - 1) % titles.size();
        if (on_change)
        {
            on_change(selected, titles[selected]);
        }
    }

    void TabsState::select_first()
    {
        if (titles.empty())
        {
            return;
        }
        selected = 0;
        if (on_change)
        {
            on_change(selected, titles[selected]);
        }
    }

    void TabsState::select_last()
    {
        if (titles.empty())
        {
            return;
        }
        selected = titles.size() - 1;
        if (on_change)
        {
            on_change(selected, titles[selected]);
        }
    }

    void TabsState::set_selected(size_t index)
    {
        if (index < titles.size())
        {
            selected = index;
            if (on_change)
            {
                on_change(selected, titles[selected]);
            }
        }
    }

    bool TabsState::handle_event(const KeyEvent& key)
    {
        if (key.is(Key::Tab) || key.is(Key::Right))
        {
            select_next();
            return true;
        }
        if (key.is(Key::BackTab) || key.is(Key::Left))
        {
            select_prev();
            return true;
        }
        if (key.is(Key::Home))
        {
            select_first();
            return true;
        }
        if (key.is(Key::End))
        {
            select_last();
            return true;
        }

        // Direct number key switching 1..9
        if (!key.ctrl && !key.alt && key.ch >= '1' && key.ch <= '9')
        {
            size_t idx = static_cast<size_t>(key.ch - '1');
            if (idx < titles.size())
            {
                set_selected(idx);
                return true;
            }
        }

        return false;
    }

    std::optional<std::string_view> TabsState::selected_title() const
    {
        if (selected < titles.size())
        {
            return std::string_view(titles[selected]);
        }
        return std::nullopt;
    }

    const std::string* TabsState::selected_title_ptr() const
    {
        if (selected < titles.size())
        {
            return &titles[selected];
        }
        return nullptr;
    }

    // =========================================================================
    // Tabs Widget Implementation
    // =========================================================================

    Tabs::Tabs(TabsState& state)
        : m_state(&state)
    {}

    Tabs::Tabs(std::vector<std::string> titles)
        : m_titles(std::move(titles))
    {}

    Tabs& Tabs::selected(size_t index)
    {
        m_selected = index;
        if (m_state)
        {
            m_state->set_selected(index);
        }
        return *this;
    }

    Tabs& Tabs::style(TabsStyle s)
    {
        m_style = s;
        return *this;
    }

    Tabs& Tabs::separator(std::string_view sep)
    {
        m_separator = std::string(sep);
        return *this;
    }

    Tabs& Tabs::separator_fg(std::string_view color)
    {
        m_separator_fg = parse_color(color);
        return *this;
    }

    Tabs& Tabs::separator_fg(uint32_t color)
    {
        m_separator_fg = color;
        return *this;
    }

    Tabs& Tabs::active_fg(std::string_view color)
    {
        m_active_fg = parse_color(color);
        return *this;
    }

    Tabs& Tabs::active_fg(uint32_t color)
    {
        m_active_fg = color;
        return *this;
    }

    Tabs& Tabs::active_bg(std::string_view color)
    {
        m_active_bg = parse_color(color);
        return *this;
    }

    Tabs& Tabs::active_bg(uint32_t color)
    {
        m_active_bg = color;
        return *this;
    }

    Tabs& Tabs::inactive_fg(std::string_view color)
    {
        m_inactive_fg = parse_color(color);
        return *this;
    }

    Tabs& Tabs::inactive_fg(uint32_t color)
    {
        m_inactive_fg = color;
        return *this;
    }

    Tabs& Tabs::inactive_bg(std::string_view color)
    {
        m_inactive_bg = parse_color(color);
        return *this;
    }

    Tabs& Tabs::inactive_bg(uint32_t color)
    {
        m_inactive_bg = color;
        return *this;
    }

    Tabs& Tabs::border_style(BorderStyle border)
    {
        m_border_style = border;
        return *this;
    }

    Tabs& Tabs::title(std::string_view title)
    {
        m_box_title = std::string(title);
        return *this;
    }

    Tabs& Tabs::title_align(TitleAlign align)
    {
        m_title_align = align;
        return *this;
    }

    void Tabs::draw(Frame& frame, Area area)
    {
        if (!area.is_valid() || area.width <= 0 || area.height <= 0)
        {
            return;
        }

        Area inner = area;
        if (m_border_style != BorderStyle::None)
        {
            Box()
                .border_style(m_border_style)
                .title(m_box_title)
                .title_align(m_title_align)
                .draw(frame, area);
            inner = area.inset(1, 1, 1, 1);
        }

        if (!inner.is_valid() || inner.width <= 0 || inner.height <= 0)
        {
            return;
        }

        const auto& titles = m_state ? m_state->titles : m_titles;
        if (titles.empty())
        {
            return;
        }

        size_t active_idx = m_state ? m_state->selected : m_selected;
        if (active_idx >= titles.size())
        {
            active_idx = titles.size() - 1;
        }

        int cur_col = inner.col;
        int row = inner.row;

        // Tracks active tab horizontal bounds for underline styling
        int active_start_col = -1;
        int active_end_col = -1;

        auto print_text = [&](std::string_view text, uint32_t fg_col, uint32_t bg_col) {
            std::string_view rem = text;
            while (!rem.empty() && cur_col < inner.right())
            {
                char32_t cp = next_codepoint(rem);
                int w = codepoint_width(cp);
                if (cur_col + w <= inner.right())
                {
                    frame.set_cell(row, cur_col, {cp, fg_col, bg_col});
                }
                cur_col += w;
            }
        };

        for (auto [i_idx, title] : std::views::enumerate(titles))
        {
            size_t i = static_cast<size_t>(i_idx);
            if (cur_col >= inner.right())
            {
                break;
            }

            bool is_active = (i == active_idx);
            uint32_t fg_color = is_active ? m_active_fg : m_inactive_fg;
            uint32_t bg_color = is_active ? m_active_bg : m_inactive_bg;

            int tab_start_x = cur_col;

            if (m_style == TabsStyle::Pill)
            {
                if (is_active)
                {
                    print_text("[ ", fg_color, bg_color);
                    print_text(title, fg_color, bg_color);
                    print_text(" ]", fg_color, bg_color);
                }
                else
                {
                    print_text("  ", fg_color, bg_color);
                    print_text(title, fg_color, bg_color);
                    print_text("  ", fg_color, bg_color);
                }
            }
            else // Bar & Underline styles
            {
                print_text(" ", fg_color, bg_color);
                print_text(title, fg_color, bg_color);
                print_text(" ", fg_color, bg_color);
            }

            int tab_end_x = cur_col;

            if (is_active)
            {
                active_start_col = tab_start_x;
                active_end_col = tab_end_x;
            }

            // Draw separator between tabs (not after the last tab)
            if (i + 1 < titles.size() && cur_col < inner.right())
            {
                if (m_style == TabsStyle::Bar)
                {
                    print_text(m_separator, m_separator_fg, m_inactive_bg);
                }
                else if (m_style == TabsStyle::Pill)
                {
                    print_text(" ", m_inactive_fg, m_inactive_bg);
                }
                else if (m_style == TabsStyle::Underline)
                {
                    print_text(" ", m_inactive_fg, m_inactive_bg);
                }
            }
        }

        // Underline style row 2
        if (m_style == TabsStyle::Underline && inner.height >= 2)
        {
            int u_row = row + 1;
            for (int c = inner.col; c < inner.right(); ++c)
            {
                if (c >= active_start_col && c < active_end_col)
                {
                    frame.set_cell(u_row, c, {U'━', m_active_bg, m_inactive_bg});
                }
                else
                {
                    frame.set_cell(u_row, c, {U'─', m_separator_fg, m_inactive_bg});
                }
            }
        }
    }
}
