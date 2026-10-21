#include "koto/list.hpp"
#include "koto/box.hpp"
#include "koto/color.hpp"
#include "koto/frame.hpp"
#include "koto/layout.hpp"
#include "koto/text.hpp"
#include "koto/utf8.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace koto
{
    ListState::ListState(std::vector<std::string> initial_items, ListNavigation nav)
        : items(std::move(initial_items)), selected(0), scroll(0), navigation(nav)
    {}

    auto ListState::set_items(std::vector<std::string> new_items) -> void
    {
        this->items = std::move(new_items);
        if (this->items.empty())
        {
            this->selected = 0;
            this->scroll = 0;
        }
        else if (this->selected >= static_cast<int>(this->items.size()))
        {
            this->selected = static_cast<int>(this->items.size()) - 1;
        }
    }

    auto ListState::selected_item() const -> std::optional<std::string_view>
    {
        if (this->selected >= 0 && this->selected < static_cast<int>(this->items.size()))
        {
            return std::string_view(this->items[static_cast<size_t>(this->selected)]);
        }
        return std::nullopt;
    }

    auto ListState::selected_item_ptr() const -> const std::string*
    {
        if (this->selected >= 0 && this->selected < static_cast<int>(this->items.size()))
        {
            return &this->items[static_cast<size_t>(this->selected)];
        }
        return nullptr;
    }

    auto ListState::set_selected(int index) -> void
    {
        if (this->items.empty())
        {
            this->selected = 0;
            return;
        }

        int size = static_cast<int>(this->items.size());
        if (this->navigation == ListNavigation::Wrap)
        {
            if (index >= size)
            {
                this->selected = 0;
            }
            else if (index < 0)
            {
                this->selected = size - 1;
            }
            else
            {
                this->selected = index;
            }
        }
        else
        {
            this->selected = std::clamp(index, 0, size - 1);
        }

        if (this->on_change)
        {
            this->on_change(this->selected, this->items[static_cast<size_t>(this->selected)]);
        }
    }

    auto ListState::select_next() -> void
    {
        if (this->items.empty()) return;
        int size = static_cast<int>(this->items.size());
        if (this->navigation == ListNavigation::Wrap)
        {
            this->set_selected((this->selected + 1) % size);
        }
        else
        {
            this->set_selected(std::min(this->selected + 1, size - 1));
        }
    }

    auto ListState::select_prev() -> void
    {
        if (this->items.empty()) return;
        int size = static_cast<int>(this->items.size());
        if (this->navigation == ListNavigation::Wrap)
        {
            this->set_selected((this->selected - 1 + size) % size);
        }
        else
        {
            this->set_selected(std::max(this->selected - 1, 0));
        }
    }

    auto ListState::select_first() -> void
    {
        if (this->items.empty()) return;
        this->set_selected(0);
    }

    auto ListState::select_last() -> void
    {
        if (this->items.empty()) return;
        this->set_selected(static_cast<int>(this->items.size()) - 1);
    }

    auto ListState::handle_event(const KeyEvent& key) -> bool
    {
        if (!this->focused || this->items.empty()) return false;

        if (key.ch == 'j' || key.is(Key::Down))
        {
            this->select_next();
            return true;
        }
        if (key.ch == 'k' || key.is(Key::Up))
        {
            this->select_prev();
            return true;
        }
        if (key.ch == 'g' || key.is(Key::Home))
        {
            this->select_first();
            return true;
        }
        if (key.ch == 'G' || key.is(Key::End))
        {
            this->select_last();
            return true;
        }

        return false;
    }

    auto ListState::handle_mouse(const MouseEvent& mouse) -> bool
    {
        if (this->items.empty()) return false;

        if (mouse.button == MouseButton::WheelUp)
        {
            this->select_prev();
            return true;
        }
        if (mouse.button == MouseButton::WheelDown)
        {
            this->select_next();
            return true;
        }

        if (mouse.action == MouseAction::Press && mouse.button == MouseButton::Left)
        {
            if (this->rendered_inner.contains(mouse.row, mouse.col))
            {
                int row_offset = mouse.row - this->rendered_inner.row;
                int target_idx = this->scroll + row_offset;
                if (target_idx >= 0 && target_idx < static_cast<int>(this->items.size()))
                {
                    this->set_selected(target_idx);
                    return true;
                }
            }
        }
        return false;
    }


    List& List::border_style(BorderStyle border)
    {
        m_border_style = border;
        return *this;
    }


    List& List::fg(std::string_view color)
    {
        m_fg = parse_color(color);
        return *this;   
    }

    List& List::bg(std::string_view color)
    {
        m_bg = parse_color(color);
        return *this;   
    }

    List& List::highlight_fg(std::string_view color)
    {
        m_highlighted_fg = parse_color(color);
        return *this;   
    }

    List& List::highlight_bg(std::string_view color)
    {
        m_highlighted_bg = parse_color(color);
        return *this;   
    }

    List& List::fg(uint32_t color)
    {
        m_fg = color;
        return *this;   
    }

    List& List::bg(uint32_t color)
    {
        m_bg = color;
        return *this;   
    }

    List& List::highlight_fg(uint32_t color)
    {
        m_highlighted_fg = color;
        return *this;   
    }

    List& List::highlight_bg(uint32_t color)
    {
        m_highlighted_bg = color;
        return *this;   
    }

    List& List::highlight_symbol(std::string symbol)
    {
        m_highlight_symbol = std::move(symbol);
        return *this;
    }

    List& List::title(std::string title)
    {
        m_title = std::move(title);
        return *this;
    }

    List& List::title_fg(std::string_view color)
    {
        m_title_fg = parse_color(color);
        return *this;
    }

    List& List::title_fg(uint32_t color)
    {
        m_title_fg = color;
        return *this;
    }

    List& List::title_bg(std::string_view color)
    {
        m_title_bg = parse_color(color);
        return *this;
    }

    List& List::title_bg(uint32_t color)
    {
        m_title_bg = color;
        return *this;
    }

    List& List::title_align(TitleAlign align)
    {
        m_title_align = align;
        return *this;
    }

    List& List::overflow(TextOverflow o)
    {
        m_overflow = o;
        return *this;
    }

    List& List::focused(bool is_focused)
    {
        m_focused = is_focused;
        m_has_focused_override = true;
        return *this;
    }

    List& List::focused_border_fg(std::string_view color)
    {
        m_focused_border_fg = parse_color(color);
        return *this;
    }

    List& List::focused_border_fg(uint32_t color)
    {
        m_focused_border_fg = color;
        return *this;
    }

    List& List::id(Id tag_id)
    {
        m_id = tag_id;
        return *this;
    }

    void List::normalize_state()
    {
        if (m_state.items.empty())
        {
            m_state.selected = 0;
            m_state.scroll = 0;
            return;
        }

        int size = static_cast<int>(m_state.items.size());
        switch (m_state.navigation)
        {
            case ListNavigation::Clamp:
            {
                m_state.selected = std::clamp(m_state.selected, 0, size - 1);
                break;
            }
            case ListNavigation::Wrap:
            {
                if (m_state.selected >= size)
                {
                    m_state.selected = 0;
                }
                else if (m_state.selected < 0)
                {
                    m_state.selected = size - 1;
                }
                break;
            }
        }
    }

    void List::draw(Frame& frame, Area area)
    {
        if (!area.is_valid())
        {
            return;
        }

        bool is_foc = m_has_focused_override ? m_focused : m_state.focused;
        Id effective_id = m_id.is_none() ? Id(&m_state) : m_id;
        frame.tag_area(area, effective_id);

        Box()
            .border_style(m_border_style)
            .bg(m_bg)
            .fg(m_fg)
            .focused(is_foc)
            .focused_border_fg(m_focused_border_fg)
            .id(effective_id)
            .filled(true)
            .title(m_title)
            .title_fg(m_title_fg)
            .title_bg(m_title_bg)
            .title_align(m_title_align)
            .draw(frame, area);

        Area inner = (m_border_style == BorderStyle::None) ? area : area.inset(1, 1, 1, 1);
        m_state.rendered_area = area;
        m_state.rendered_inner = inner;
        
        if (!inner.is_valid() || m_state.items.empty())
        {
            return;    
        }

        int visible_rows = inner.height;

        this->normalize_state();

        if (m_state.selected < m_state.scroll)
        {
            m_state.scroll = m_state.selected;
        }

        if (m_state.selected >= m_state.scroll + visible_rows)
        {
            m_state.scroll = m_state.selected - visible_rows + 1;
        }

        for (int i = 0; i < visible_rows; i++)
        {
            int item_idx = i + m_state.scroll;
            if (item_idx >= static_cast<int>(m_state.items.size()))
            {
                break;
            }

            Area row_area = inner.set_row(inner.row + i).set_height(1);
            bool is_selected = (m_state.selected == item_idx);

            uint32_t current_fg = is_selected ? m_highlighted_fg : m_fg;
            uint32_t current_bg = is_selected ? m_highlighted_bg : m_bg;

            // Fill full row width with background color
            if (current_bg != COLOR_DEFAULT)
            {
                for (int c = row_area.col; c < row_area.right(); c++)
                {
                    frame.set_cell(row_area.row, c, {U' ', current_fg, current_bg});
                }
            }

            std::string line_text;
            if (!m_highlight_symbol.empty())
            {
                if (is_selected)
                {
                    line_text = m_highlight_symbol + m_state.items[static_cast<size_t>(item_idx)];
                }
                else
                {
                    int sym_w = text_width(m_highlight_symbol);
                    line_text = std::string(static_cast<size_t>(sym_w > 0 ? sym_w : 0), ' ') + m_state.items[static_cast<size_t>(item_idx)];
                }
            }
            else
            {
                line_text = m_state.items[static_cast<size_t>(item_idx)];
            }

            Text(line_text)
                .fg(current_fg)
                .bg(current_bg)
                .overflow(m_overflow)
                .draw(frame, row_area);
        }
    }
}