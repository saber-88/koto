#include "eztui/terminal.hpp"
#include <cstdint>
#include <cstdio>
#include <termios.h>
#include <unistd.h>
#include <fmt/base.h>
#include <sys/ioctl.h>

namespace eztui 
{

    void Terminal::enter_raw_mode()
    {

        tcgetattr(STDIN_FILENO, &og_terminal);
        struct termios raw = og_terminal;
        raw.c_lflag &= ~(ECHO | ICANON );
        tcsetattr(STDIN_FILENO, TCSANOW, &raw); 
        std::fflush(stdout);
        
    }

    void Terminal::restore_terminal()
    {
        
        tcsetattr(STDIN_FILENO, TCSANOW, &og_terminal);
        std::fflush(stdout);
        
    }

    void Terminal::enter_alt_creen()
    {
        
        fmt::print("\033[?1049h");
        std::fflush(stdout);
        
    }

    void Terminal::exit_alt_Screen()
    {
        
        fmt::print("\033[?1049l");
        std::fflush(stdout);
    }

    void Terminal::show_cursor()
    {

        fmt::print("\033[?25h");
        std::fflush(stdout);

    }

    void Terminal::hide_cursor()
    {

        fmt::print("\033[?25l");
        std::fflush(stdout);

    }

    void Terminal::set_dimensions()
    {

        struct winsize w;
        if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        {
            
            m_rows = w.ws_row;
            m_cols = w.ws_col;

        }

    }

    Terminal::Terminal()
    {
        
        set_dimensions();
        enter_raw_mode();
        enter_alt_creen();
        
    }

    Terminal::~Terminal()
    {

        show_cursor();
        exit_alt_Screen();
        restore_terminal();

    }

    uint32_t Terminal::get_rows() const
    {

        return m_rows;

    }

    uint32_t Terminal::get_cols() const
    {

        return m_cols;

    }

}