#pragma once

#include "koto/box.hpp"
#include "koto/color.hpp"
#include "koto/event.hpp"
#include "koto/layout.hpp"
#include "koto/widget.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace koto
{
    struct TextInputState
    {
        using KeyInterceptor = std::function<bool(TextInputState&, const KeyEvent&)>;
        using InputFilter = std::function<bool(std::string_view current_text, char32_t new_char)>;

        std::string value;
        size_t cursor = 0;        // byte offset in value
        int scroll_offset = 0;    // visual column offset for horizontal scroll
        bool focused = true;

        KeyInterceptor on_key_handler = nullptr;
        InputFilter input_filter = nullptr;

        TextInputState() = default;
        TextInputState(std::string_view initial_text);

        // High-level event dispatcher (runs interceptor hook first, then defaults)
        auto handle_event(const KeyEvent& key) -> bool;

        // Customization hooks
        auto on_key(KeyInterceptor handler) -> TextInputState&;
        auto set_filter(InputFilter filter) -> TextInputState&;

        // Granular primitive mutations (can be called directly for custom keymaps/vim modes)
        auto insert(char32_t cp) -> bool;
        auto insert(std::string_view text) -> bool;
        auto backspace() -> bool;
        auto delete_forward() -> bool;
        auto delete_word() -> bool;
        auto move_left() -> bool;
        auto move_right() -> bool;
        auto move_home() -> bool;
        auto move_end() -> bool;

        auto set_text(std::string_view text) -> void;
        auto text() const -> std::string_view;
        auto clear() -> void;
    };

    class TextInput : public Widget
    {
        public:
            TextInput(TextInputState& state);

            TextInput& placeholder(std::string_view placeholder);
            TextInput& title(std::string_view title);
            TextInput& border_style(BorderStyle style);
            TextInput& fg(std::string_view color);
            TextInput& bg(std::string_view color);
            TextInput& fg(uint32_t color);
            TextInput& bg(uint32_t color);
            TextInput& placeholder_fg(std::string_view color);
            TextInput& placeholder_fg(uint32_t color);
            TextInput& focused_border_fg(std::string_view color);
            TextInput& focused_border_fg(uint32_t color);

            void draw(Frame& frame, Area area) override;

        private:
            TextInputState& m_state;
            std::string m_placeholder;
            std::string m_title;
            BorderStyle m_border_style = BorderStyle::Single;
            uint32_t m_fg = COLOR_DEFAULT;
            uint32_t m_bg = COLOR_DEFAULT;
            uint32_t m_placeholder_fg = 0x00888888;
            uint32_t m_focused_border_fg = 0x0055FFFF;
    };
}
