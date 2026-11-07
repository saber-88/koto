#pragma once
#include "koto/color.hpp"
#include <cstdint>
#include <expected>
#include <string_view>
#include <termios.h>

namespace koto
{
    enum class TerminalError : uint8_t
    {
        None = 0,
        NotATty,              // Stdin or stdout is not a TTY (piped or redirected)
        GetAttrFailed,        // tcgetattr() failed
        SetAttrFailed,        // tcsetattr() failed
    };

    struct TerminalErrorInfo
    {
        TerminalError code = TerminalError::None;
        int system_errno = 0;

        constexpr std::string_view message() const noexcept
        {
            switch (code)
            {
                case TerminalError::None:
                    return "Success";
                case TerminalError::NotATty:
                    return "Standard input or standard output is not a terminal (TTY)";
                case TerminalError::GetAttrFailed:
                    return "Failed to get terminal attributes (tcgetattr failed)";
                case TerminalError::SetAttrFailed:
                    return "Failed to set terminal raw mode attributes (tcsetattr failed)";
            }
            return "Unknown terminal error";
        }
    };

    class Terminal
    {
        public:
            Terminal();
            ~Terminal();

            Terminal(const Terminal&) = delete;
            Terminal& operator=(const Terminal&) = delete;

            Terminal(Terminal&&) = delete;
            Terminal& operator=(Terminal&&) = delete;
            
            bool is_tty() const noexcept { return m_is_tty; }
            bool in_raw_mode() const noexcept { return m_in_raw_mode; }
            ColorMode color_mode() const noexcept { return m_color_mode; }
            Terminal& color_mode(ColorMode mode) noexcept { m_color_mode = mode; return *this; }

            // Enter raw mode, alternate screen, and input enhancements
            auto enter_raw_mode() -> std::expected<void, TerminalErrorInfo>;
            auto restore_terminal() -> void;

            auto get_cols() const -> int;
            auto get_rows() const -> int;
            int cols() const noexcept { return m_cols; }
            int rows() const noexcept { return m_rows; }
            int width() const noexcept { return m_cols; }
            int height() const noexcept { return m_rows; }
            auto set_dimensions(int r, int c) noexcept -> void { m_rows = r; m_cols = c; }

            auto show_cursor() const -> void;
            auto hide_cursor() const -> void;

            auto enable_bracketed_paste() const -> void;
            auto disable_bracketed_paste() const -> void;
            auto enable_kitty_keyboard() const -> void;
            auto disable_kitty_keyboard() const -> void;
            auto enable_mouse_capture() const -> void;
            auto disable_mouse_capture() const -> void;
            auto enable_mouse_drag() const -> void;
            auto disable_mouse_drag() const -> void;
            
        private:
            struct termios og_terminal{};
            int m_rows = 24;
            int m_cols = 80;
            bool m_is_tty = false;
            bool m_in_raw_mode = false;
            ColorMode m_color_mode = ColorMode::TrueColor;

            auto set_dimensions()   -> void;
            auto enter_alt_screen() -> void;
            auto exit_alt_screen()  -> void;
    };
}
