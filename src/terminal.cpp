#include "koto/terminal.hpp"
#include <clocale>
#include <cstdio>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

namespace koto 
{

    auto Terminal::enter_raw_mode() -> void
    {
        std::setlocale(LC_ALL,"");
        tcgetattr(STDIN_FILENO, &og_terminal);
        struct termios raw = og_terminal;
        raw.c_lflag &= ~static_cast<tcflag_t>((ECHO | ICANON | ISIG ));
        raw.c_iflag &= ~static_cast<tcflag_t>((IXON));
        tcsetattr(STDIN_FILENO, TCSANOW, &raw); 
        std::fflush(stdout);

    }

    auto Terminal::restore_terminal() -> void
    {
        
        tcsetattr(STDIN_FILENO, TCSANOW, &og_terminal);
        show_cursor();
        exit_alt_screen();
        std::fflush(stdout);
        
    }

    auto Terminal::enter_alt_screen() -> void
    {
        
        write(STDOUT_FILENO, "\033[?1049h", 8);
        std::fflush(stdout);
        
    }

    auto Terminal::exit_alt_screen() -> void
    {
        
        write(STDOUT_FILENO, "\033[?1049l", 8);
        std::fflush(stdout);
    }

    auto Terminal::show_cursor() const -> void
    {

        write(STDOUT_FILENO, "\033[?25h", 6);
        std::fflush(stdout);
        
    }
    
    auto Terminal::hide_cursor() const -> void
    {
        
        write(STDOUT_FILENO, "\033[?25l", 6);
        std::fflush(stdout);

    }

    auto Terminal::set_dimensions() -> void
    {

        struct winsize w;
        if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        {
            
            m_rows = w.ws_row;
            m_cols = w.ws_col;

        }
        else
        {
            // sensible fallback when not attached to a terminal
            m_rows = 24;
            m_cols = 80;
        }

    }

    Terminal::Terminal()
    {
        
        set_dimensions();
        enter_raw_mode();
        enter_alt_screen();
        
    }

    Terminal::~Terminal()
    {

        restore_terminal();

    }

    auto Terminal::get_rows() const -> int
    {

        return m_rows;

    }

    auto Terminal::get_cols() const -> int
    {

        return m_cols;

    }

}