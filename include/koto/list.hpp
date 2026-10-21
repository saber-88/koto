#pragma once

#include "koto/box.hpp"
#include "koto/color.hpp"
#include "koto/layout.hpp"
#include "koto/text.hpp"
#include "koto/widget.hpp"
#include "koto/event.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#include <string>
#include <vector>

namespace koto
{
    enum class ListNavigation
    {
        Clamp,
        Wrap,
    };

    struct ListState
    {
        std::vector<std::string> items;
        int selected = 0;
        int scroll = 0;
        bool focused = false;
        ListNavigation navigation = ListNavigation::Clamp;
        std::function<void(int, const std::string&)> on_change = nullptr;

        ListState() = default;
        ListState(std::vector<std::string> initial_items, ListNavigation nav = ListNavigation::Clamp);

        auto select_next() -> void;
        auto select_prev() -> void;
        auto select_first() -> void;
        auto select_last() -> void;
        auto set_selected(int index) -> void;

        auto handle_event(const KeyEvent& key) -> bool;
        auto set_items(std::vector<std::string> new_items) -> void;
        auto selected_item() const -> std::optional<std::string_view>;
        auto selected_item_ptr() const -> const std::string*;
    };
    
    class List : public Widget
    {
        
        
        public:
            List& fg(std::string_view color);
            List& bg(std::string_view color);
            List& highlight_fg(std::string_view color);
            List& highlight_bg(std::string_view color);
            List& fg(uint32_t color);
            List& bg(uint32_t color);
            List& highlight_fg(uint32_t color);
            List& highlight_bg(uint32_t color);
            List& border_style(BorderStyle border);
            List& highlight_symbol(std::string symbol);
            List& title(std::string title);
            List& title_fg(std::string_view color);
            List& title_fg(uint32_t color);
            List& title_bg(std::string_view color);
            List& title_bg(uint32_t color);
            List& title_align(TitleAlign align);
            List& overflow(TextOverflow o);
            void draw(Frame &frame, Area area) override;
            explicit List(ListState& state) : m_state(state) {}

        private:
            ListState& m_state;
            uint32_t m_fg = COLOR_DEFAULT;
            uint32_t m_bg = COLOR_DEFAULT;
            uint32_t m_highlighted_fg = COLOR_DEFAULT;
            uint32_t m_highlighted_bg = COLOR_DEFAULT;
            BorderStyle m_border_style = BorderStyle::None;
            std::string m_highlight_symbol = "";
            std::string m_title = "";
            uint32_t m_title_fg = COLOR_DEFAULT;
            uint32_t m_title_bg = COLOR_DEFAULT;
            TitleAlign m_title_align = TitleAlign::Left;
            TextOverflow m_overflow = TextOverflow::Ellipsis;
            
            void normalize_state();
    };

}
