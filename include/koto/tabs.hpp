#pragma once

#include "koto/box.hpp"
#include "koto/color.hpp"
#include "koto/event.hpp"
#include "koto/frame.hpp"
#include "koto/layout.hpp"
#include "koto/widget.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace koto
{
    enum class TabsStyle
    {
        Bar,        // " Home │ Output │ Settings "
        Pill,       // " [ Home ]  [ Output ]  [ Settings ] "
        Underline   // Top line tabs, bottom line underline bar
    };

    struct TabsState
    {
        std::vector<std::string> titles;
        size_t selected = 0;
        std::function<void(size_t, const std::string&)> on_change = nullptr;

        TabsState() = default;
        TabsState(std::vector<std::string> initial_titles, size_t initial_selected = 0);

        void select_next();
        void select_prev();
        void select_first();
        void select_last();
        void set_selected(size_t index);
        bool handle_event(const KeyEvent& key);
        std::optional<std::string_view> selected_title() const;
        const std::string* selected_title_ptr() const;
    };

    class Tabs : public Widget
    {
    public:
        Tabs() = default;
        explicit Tabs(TabsState& state);
        explicit Tabs(std::vector<std::string> titles);

        Tabs& selected(size_t index);
        Tabs& style(TabsStyle s);
        Tabs& separator(std::string_view sep);
        Tabs& separator_fg(std::string_view color);
        Tabs& separator_fg(uint32_t color);

        Tabs& active_fg(std::string_view color);
        Tabs& active_fg(uint32_t color);
        Tabs& active_bg(std::string_view color);
        Tabs& active_bg(uint32_t color);

        Tabs& inactive_fg(std::string_view color);
        Tabs& inactive_fg(uint32_t color);
        Tabs& inactive_bg(std::string_view color);
        Tabs& inactive_bg(uint32_t color);

        Tabs& border_style(BorderStyle border);
        Tabs& title(std::string_view title);
        Tabs& title_align(TitleAlign align);

        void draw(Frame& frame, Area area) override;

    private:
        TabsState* m_state = nullptr;
        std::vector<std::string> m_titles;
        size_t m_selected = 0;
        TabsStyle m_style = TabsStyle::Bar;
        std::string m_separator = "│";
        uint32_t m_separator_fg = Color::DarkGray;
        uint32_t m_active_fg = Color::Black;
        uint32_t m_active_bg = Color::Cyan;
        uint32_t m_inactive_fg = Color::White;
        uint32_t m_inactive_bg = COLOR_DEFAULT;
        BorderStyle m_border_style = BorderStyle::None;
        std::string m_box_title;
        TitleAlign m_title_align = TitleAlign::Left;
    };
}
