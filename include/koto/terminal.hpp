#pragma once
#include <cstdint>
#include <termios.h>

namespace koto
{

    class Terminal
    {

        private:
        
            struct termios og_terminal;
            uint32_t m_rows , m_cols;
            auto set_dimensions()   -> void;
            auto enter_raw_mode()   -> void;
            auto restore_terminal() -> void;
            auto enter_alt_screen()  -> void;
            auto exit_alt_screen()  -> void;
            
        public:
        
            Terminal();
            ~Terminal();

            auto get_cols() const -> uint32_t;
            auto get_rows() const -> uint32_t;

            auto show_cursor() const -> void;
            auto hide_cursor() const -> void;
    };
}