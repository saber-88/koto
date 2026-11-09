#include "koto/button.hpp"
#include "koto/box.hpp"
#include "koto/color.hpp"
#include "koto/frame.hpp"
#include "koto/text.hpp"

namespace koto
{
    Button::Button(std::string_view label)
        : m_label(label)
    {}

    Button::Button(std::string_view label, std::function<void()> on_click)
        : m_label(label), m_on_click(std::move(on_click))
    {}

    Button& Button::label(std::string_view label)
    {
        m_label = label;
        return *this;
    }

    Button& Button::border_style(BorderStyle style)
    {
        m_border_style = style;
        return *this;
    }

    Button& Button::fg(Color color)
    {
        m_fg = color.value;
        m_has_fg = true;
        if (!m_has_label_fg)
        {
            m_label_fg = color.value;
        }
        return *this;
    }

    Button& Button::bg(Color color)
    {
        m_bg = color.value;
        return *this;
    }

    Button& Button::label_fg(Color color)
    {
        m_label_fg = color.value;
        m_has_label_fg = true;
        return *this;
    }

    Button& Button::focused(bool is_foc)
    {
        m_is_focused = is_foc;
        m_manual_focus_set = true;
        return *this;
    }

    Button& Button::focused_fg(Color color)
    {
        m_focused_fg = color.value;
        m_has_focused_fg = true;
        if (!m_has_focused_label_fg)
        {
            m_focused_label_fg = color.value;
        }
        if (!m_has_focused_border_fg)
        {
            m_focused_border_fg = color.value;
        }
        return *this;
    }

    Button& Button::focused_bg(Color color)
    {
        m_focused_bg = color.value;
        m_has_focused_bg = true;
        return *this;
    }

    Button& Button::focused_border_fg(Color color)
    {
        m_focused_border_fg = color.value;
        m_has_focused_border_fg = true;
        return *this;
    }

    Button& Button::focused_label_fg(Color color)
    {
        m_focused_label_fg = color.value;
        m_has_focused_label_fg = true;
        return *this;
    }

    Button& Button::id(Id id)
    {
        m_id = id;
        return *this;
    }

    Button& Button::focus(FocusManager& fm)
    {
        m_fm = &fm;
        return *this;
    }

    Button& Button::on_click(std::function<void()> callback)
    {
        m_on_click = std::move(callback);
        return *this;
    }

    void Button::draw(Frame& frame, Area area)
    {
        if (m_fm != nullptr && !m_id.is_none())
        {
            if (m_on_click)
            {
                Id current_modal = m_fm->active_modal_scope();
                FocusTarget target{
                    .id = m_fm->scoped_id(m_id),
                    .bounds = area,
                    .modal_scope = current_modal,
                    .enabled = true,
                    .visible = true,
                    .set_focus = nullptr,
                    .handle_key = [cb = m_on_click](const KeyEvent& k) {
                        if (k.is(Key::Enter) || k.ch == ' ')
                        {
                            cb();
                            return true;
                        }
                        return false;
                    },
                    .handle_mouse = [cb = m_on_click](const MouseEvent& m) {
                        if (m.is_left_press())
                        {
                            cb();
                            return true;
                        }
                        return false;
                    },
                    .handle_paste = nullptr,
                    .on_focus = nullptr,
                    .on_blur = nullptr
                };
                m_fm->register_target(std::move(target));
            }

            if (!m_manual_focus_set)
            {
                m_is_focused = m_fm->is_focused(m_id);
            }
        }

        uint32_t border_col = m_is_focused ? m_focused_border_fg : m_fg;
        uint32_t bg_col     = (m_is_focused && m_has_focused_bg) ? m_focused_bg : m_bg;
        uint32_t text_col   = m_is_focused ? m_focused_label_fg : m_label_fg;

        Box()
            .border_style(m_border_style)
            .fg(border_col)
            .bg(bg_col)
            .id(m_id)
            .focused(m_is_focused)
            .focused_border_fg(m_focused_border_fg)
            .filled(true)
            .draw(frame, area);

        if (!m_label.empty() && area.height > 0 && area.width > 0)
        {
            Area text_area = area;
            if (area.height > 2 && area.width > 2 && m_border_style != BorderStyle::None)
            {
                int target_row = area.row + (area.height - 1) / 2;
                text_area = Area{target_row, area.col + 1, area.width - 2, 1};
            }
            else if (m_border_style == BorderStyle::None)
            {
                int target_row = area.row + (area.height - 1) / 2;
                text_area = Area{target_row, area.col, area.width, 1};
            }

            Text(m_label)
                .fg(text_col)
                .bg(bg_col)
                .clip()
                .align(TextAlign::Center)
                .draw(frame, text_area);
        }
    }
}
