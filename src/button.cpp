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

    Button& Button::fg(uint32_t color)
    {
        m_fg = color;
        return *this;
    }

    Button& Button::fg(std::string_view color)
    {
        m_fg = parse_color(color);
        return *this;
    }

    Button& Button::bg(uint32_t color)
    {
        m_bg = color;
        return *this;
    }

    Button& Button::bg(std::string_view color)
    {
        m_bg = parse_color(color);
        return *this;
    }

    Button& Button::label_fg(uint32_t color)
    {
        m_label_fg = color;
        return *this;
    }

    Button& Button::label_fg(std::string_view color)
    {
        m_label_fg = parse_color(color);
        return *this;
    }

    Button& Button::focused(bool is_foc)
    {
        m_is_focused = is_foc;
        m_manual_focus_set = true;
        return *this;
    }

    Button& Button::focused_fg(uint32_t color)
    {
        m_focused_fg = color;
        return *this;
    }

    Button& Button::focused_fg(std::string_view color)
    {
        m_focused_fg = parse_color(color);
        return *this;
    }

    Button& Button::focused_bg(uint32_t color)
    {
        m_focused_bg = color;
        m_has_focused_bg = true;
        return *this;
    }

    Button& Button::focused_bg(std::string_view color)
    {
        m_focused_bg = parse_color(color);
        m_has_focused_bg = true;
        return *this;
    }

    Button& Button::focused_border_fg(uint32_t color)
    {
        m_focused_border_fg = color;
        return *this;
    }

    Button& Button::focused_border_fg(std::string_view color)
    {
        m_focused_border_fg = parse_color(color);
        return *this;
    }

    Button& Button::focused_label_fg(uint32_t color)
    {
        m_focused_label_fg = color;
        return *this;
    }

    Button& Button::focused_label_fg(std::string_view color)
    {
        m_focused_label_fg = parse_color(color);
        return *this;
    }

    Button& Button::id(Id id)
    {
        m_id = id;
        return *this;
    }

    Button& Button::id(std::string_view id_str)
    {
        m_id = Id(id_str);
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
                    .set_focus = nullptr,
                    .handle_key = [cb = m_on_click](const KeyEvent& k) {
                        if (k.is(Key::Enter) || k.ch == ' ')
                        {
                            cb();
                            return true;
                        }
                        return false;
                    },
                    .handle_mouse = nullptr,
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
                text_area = area.inset(1, 1, 1, 1);
            }

            Text(m_label)
                .fg(text_col)
                .bg(bg_col)
                .align(TextAlign::Center)
                .draw(frame, text_area);
        }
    }
}
