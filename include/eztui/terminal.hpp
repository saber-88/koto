#pragma once
#include <cstdint>
#include <termios.h>

namespace eztui{

    class Terminal{

        private:
        
            struct termios og_terminal;
            uint32_t m_rows , m_cols;
            void set_dimensions();
            void enter_raw_mode();
            void restore_terminal();
            void enter_alt_creen();
            void exit_alt_Screen();
            
        public:
        
            Terminal();
            ~Terminal();
            uint32_t get_cols() const;
            uint32_t get_rows() const;
            void show_cursor();
            void hide_cursor();
    };
}