#include "koto/terminal.hpp"
#include "koto/io.hpp"
#include <clocale>
#include <csignal>
#include <cstdio>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

namespace
{
    termios g_saved{};
    volatile std::sig_atomic_t g_saved_valid = 0;
    struct sigaction g_old_term{}, g_old_hup{};
    void restore_minimal() noexcept
    {        
        constexpr char seq[] = "\033[?1006l\033[?1002l\033[?1000l"   // mouse off
                               "\033[<u"                              // pop kitty keyboard
                               "\033[?2004l"                          // bracketed paste off
                               "\033[?25h"                            // show cursor
                               "\033[?1049l";                         // leave alt screen
        const int saved_errno = errno;      // a handler must not clobber errno
        static_cast<void>(koto::write_all(STDOUT_FILENO, std::string_view(seq, sizeof(seq) - 1)));
        if (g_saved_valid) tcsetattr(STDIN_FILENO, TCSANOW, &g_saved);
        errno = saved_errno;
    }

    void on_fatal_signal(int sig)
    {
        restore_minimal();
        raise(sig);
    }

    void install_handlers()
    {
        struct sigaction sa{};
        sa.sa_handler = on_fatal_signal;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = static_cast<int>(SA_RESETHAND);
        sigaction(SIGTERM, &sa, &g_old_term);
        sigaction(SIGHUP, &sa, &g_old_hup);
        
    }
    void uninstall_handlers()
    {
        sigaction(SIGTERM, &g_old_term, nullptr);
        sigaction(SIGHUP,  &g_old_hup,  nullptr);
        g_saved_valid = 0;
    }
}
namespace koto 
{

    auto Terminal::enter_raw_mode() -> void
    {
        std::setlocale(LC_ALL,"");
        tcgetattr(STDIN_FILENO, &og_terminal);
        g_saved = og_terminal;
        g_saved_valid = 1;
        struct termios raw = og_terminal;
        raw.c_lflag &= ~static_cast<tcflag_t>((ECHO | ICANON | ISIG ));
        raw.c_iflag &= ~static_cast<tcflag_t>((IXON));
        tcsetattr(STDIN_FILENO, TCSANOW, &raw); 
        std::fflush(stdout);

    }

    auto Terminal::restore_terminal() -> void
    {
        // disable_mouse_capture();
        // disable_kitty_keyboard();
        // disable_bracketed_paste();
        // tcsetattr(STDIN_FILENO, TCSANOW, &og_terminal);
        // show_cursor();
        // exit_alt_screen();
        // std::fflush(stdout);
        restore_minimal();
        uninstall_handlers();
    }

    auto Terminal::enter_alt_screen() -> void
    {
        static_cast<void>(write_all(STDOUT_FILENO, "\033[?1049h"));
        std::fflush(stdout);
        
    }
    
    auto Terminal::exit_alt_screen() -> void
    {
        
        static_cast<void>(write_all(STDOUT_FILENO, "\033[?1049l"));
        
        std::fflush(stdout);
    }

    auto Terminal::show_cursor() const -> void
    {

        static_cast<void>(write_all(STDOUT_FILENO, "\033[?25h"));
        std::fflush(stdout);
        
    }
    
    auto Terminal::hide_cursor() const -> void
    {
        
        static_cast<void>(write_all(STDOUT_FILENO, "\033[?25l"));
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

    auto Terminal::enable_bracketed_paste() const -> void
    {
        static_cast<void>(write_all(STDOUT_FILENO, "\033[?2004h"));
        std::fflush(stdout);
    }

    auto Terminal::disable_bracketed_paste() const -> void
    {
        static_cast<void>(write_all(STDOUT_FILENO, "\033[?2004l"));
        std::fflush(stdout);
    }

    auto Terminal::enable_kitty_keyboard() const -> void
    {
        // Push Kitty keyboard flags: 1 = DISAMBIGUATE_ESCAPE_CODES
        static_cast<void>(write_all(STDOUT_FILENO, "\033[>1u"));
        std::fflush(stdout);
    }

    auto Terminal::disable_kitty_keyboard() const -> void
    {
        // Pop Kitty keyboard flags
        static_cast<void>(write_all(STDOUT_FILENO, "\033[<u"));
        std::fflush(stdout);
    }

    auto Terminal::enable_mouse_capture() const -> void
    {
        // 1000h: standard click press/release reporting (no motion spam)
        // 1006h: SGR extended mode
        static_cast<void>(write_all(STDOUT_FILENO, "\033[?1000h\033[?1006h"));
        std::fflush(stdout);
    }

    auto Terminal::disable_mouse_capture() const -> void
    {
        static_cast<void>(write_all(STDOUT_FILENO, "\033[?1006l\033[?1002l\033[?1000l"));
        std::fflush(stdout);
    }

    auto Terminal::enable_mouse_drag() const -> void
    {
        // 1002h: button event reporting (motion while button pressed)
        static_cast<void>(write_all(STDOUT_FILENO, "\033[?1002h"));
        std::fflush(stdout);
    }

    auto Terminal::disable_mouse_drag() const -> void
    {
        static_cast<void>(write_all(STDOUT_FILENO, "\033[?1002l"));
        std::fflush(stdout);
    }

    Terminal::Terminal()
    {
        install_handlers();   
        set_dimensions();
        enter_raw_mode();
        enter_alt_screen();
        enable_bracketed_paste();
        enable_kitty_keyboard();
        
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

} // namespace koto
