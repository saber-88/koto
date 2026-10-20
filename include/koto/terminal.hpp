#pragma once
#include <termios.h>

namespace koto
{

    class Terminal
    {

        public:
        
            Terminal();
            ~Terminal();

            Terminal(const Terminal&) = delete;
            Terminal& operator=(const Terminal&) = delete;

            Terminal(Terminal&&) = delete;
            Terminal& operator=(Terminal&&) = delete;
            
            auto get_cols() const -> int;
            auto get_rows() const -> int;

            auto show_cursor() const -> void;
            auto hide_cursor() const -> void;

            auto enable_bracketed_paste() const -> void;
            auto disable_bracketed_paste() const -> void;
            auto enable_kitty_keyboard() const -> void;
            auto disable_kitty_keyboard() const -> void;
            
        private:
        
            struct termios og_terminal;
            int m_rows , m_cols;
            auto set_dimensions()   -> void;
            auto enter_raw_mode()   -> void;
            auto restore_terminal() -> void;
            auto enter_alt_screen()  -> void;
            auto exit_alt_screen()  -> void;
            
    };
}