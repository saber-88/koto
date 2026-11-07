#pragma once

#include "koto/box.hpp"
#include "koto/color.hpp"
#include "koto/event.hpp"
#include "koto/focus.hpp"
#include "koto/frame.hpp"
#include "koto/id.hpp"
#include "koto/layout.hpp"
#include "koto/widget.hpp"
#include <functional>
#include <string>
#include <string_view>

namespace koto
{
    class Button : public Widget
    {
    public:
        Button() = default;
        explicit Button(std::string_view label);
        Button(std::string_view label, std::function<void()> on_click);

        Button& label(std::string_view label);
        Button& border_style(BorderStyle style);

        // Unfocused styling
        Button& fg(Color color);
        Button& bg(Color color);
        Button& label_fg(Color color);

        // Focused styling
        Button& focused(bool is_foc);
        Button& focused_fg(Color color);
        Button& focused_bg(Color color);
        Button& focused_border_fg(Color color);
        Button& focused_label_fg(Color color);

        // Identification & Focus integration
        Button& id(Id id);
        Button& focus(FocusManager& fm);

        // Action callback (triggered by click or Enter/Space)
        Button& on_click(std::function<void()> callback);

        void draw(Frame& frame, Area area) override;

    private:
        std::string m_label;
        BorderStyle m_border_style = BorderStyle::Rounded;
        uint32_t m_fg = Color::DarkGray;
        uint32_t m_bg = COLOR_DEFAULT;
        uint32_t m_label_fg = Color::LightGray;

        bool m_is_focused = false;
        bool m_manual_focus_set = false;
        bool m_has_focused_bg = false;
        bool m_has_focused_fg = false;
        bool m_has_focused_border_fg = false;
        bool m_has_focused_label_fg = false;
        bool m_has_label_fg = false;
        bool m_has_fg = false;

        uint32_t m_focused_fg = Color::Cyan;
        uint32_t m_focused_bg = Color::DarkGray;
        uint32_t m_focused_border_fg = Color::Cyan;
        uint32_t m_focused_label_fg = Color::White;

        Id m_id = Id::none();
        FocusManager* m_fm = nullptr;
        std::function<void()> m_on_click = nullptr;
    };
}
