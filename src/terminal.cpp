#include "koto/terminal.hpp"
#include "koto/io.hpp"
#include <cerrno>
#include <clocale>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

namespace
{
    termios g_saved{};
    volatile std::sig_atomic_t g_saved_valid = 0;
    struct sigaction g_old_term{}, g_old_hup{}, g_old_int{}, g_old_quit{};
    bool g_atexit_registered = false;

    void restore_minimal() noexcept
    {        
        constexpr char seq[] = "\033[?1006l\033[?1003l\033[?1002l\033[?1000l"   // mouse off
                               "\033[<u"                              // pop kitty keyboard
                               "\033[?2004l"                          // bracketed paste off
                               "\033[?25h"                            // show cursor
                               "\033[?1049l";                         // leave alt screen
        const int saved_errno = errno;      // a handler must not clobber errno
        static_cast<void>(koto::write_all(STDOUT_FILENO, std::string_view(seq, sizeof(seq) - 1)));
        if (g_saved_valid) tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_saved);
        errno = saved_errno;
    }

    void on_fatal_signal(int sig)
    {
        restore_minimal();
        raise(sig);
    }

    void install_handlers()
    {
        if (!g_atexit_registered)
        {
            std::atexit(restore_minimal);
            g_atexit_registered = true;
        }

        struct sigaction sa{};
        sa.sa_handler = on_fatal_signal;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = static_cast<int>(SA_RESETHAND);
        sigaction(SIGTERM, &sa, &g_old_term);
        sigaction(SIGHUP,  &sa, &g_old_hup);
        sigaction(SIGINT,  &sa, &g_old_int);
        sigaction(SIGQUIT, &sa, &g_old_quit);
    }

    void uninstall_handlers()
    {
        sigaction(SIGTERM, &g_old_term, nullptr);
        sigaction(SIGHUP,  &g_old_hup,  nullptr);
        sigaction(SIGINT,  &g_old_int,  nullptr);
        sigaction(SIGQUIT, &g_old_quit, nullptr);
        g_saved_valid = 0;
    }
}

namespace koto 
{
    auto Terminal::enter_raw_mode() -> std::expected<void, TerminalErrorInfo>
    {
        if (m_in_raw_mode)
        {
            return {};
        }

        if (isatty(STDIN_FILENO) == 0 || isatty(STDOUT_FILENO) == 0)
        {
            m_is_tty = false;
            return std::unexpected(TerminalErrorInfo{
                .code = TerminalError::NotATty,
                .system_errno = ENOTTY
            });
        }

        m_is_tty = true;
        std::setlocale(LC_ALL, "");

        if (tcgetattr(STDIN_FILENO, &og_terminal) != 0)
        {
            return std::unexpected(TerminalErrorInfo{
                .code = TerminalError::GetAttrFailed,
                .system_errno = errno
            });
        }

        g_saved = og_terminal;
        g_saved_valid = 1;
        install_handlers();

        struct termios raw = og_terminal;
        raw.c_lflag &= ~static_cast<tcflag_t>(ECHO | ICANON | ISIG | IEXTEN);
        raw.c_iflag &= ~static_cast<tcflag_t>(IXON | ICRNL);

        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0)
        {
            return std::unexpected(TerminalErrorInfo{
                .code = TerminalError::SetAttrFailed,
                .system_errno = errno
            });
        }

        enter_alt_screen();
        enable_bracketed_paste();
        enable_kitty_keyboard();
        std::fflush(stdout);

        m_in_raw_mode = true;
        return {};
    }

    auto Terminal::restore_terminal() -> void
    {
        if (!m_in_raw_mode)
        {
            return;
        }
        restore_minimal();
        uninstall_handlers();
        m_in_raw_mode = false;
    }

    auto Terminal::enter_alt_screen() -> void
    {
        if (m_is_tty)
        {
            static_cast<void>(write_all(STDOUT_FILENO, "\033[?1049h"));
            std::fflush(stdout);
        }
    }
    
    auto Terminal::exit_alt_screen() -> void
    {
        if (m_is_tty)
        {
            static_cast<void>(write_all(STDOUT_FILENO, "\033[?1049l"));
            std::fflush(stdout);
        }
    }

    auto Terminal::show_cursor() const -> void
    {
        if (m_is_tty)
        {
            static_cast<void>(write_all(STDOUT_FILENO, "\033[?25h"));
            std::fflush(stdout);
        }
    }
    
    auto Terminal::hide_cursor() const -> void
    {
        if (m_is_tty)
        {
            static_cast<void>(write_all(STDOUT_FILENO, "\033[?25l"));
            std::fflush(stdout);
        }
    }

    auto Terminal::set_dimensions() -> void
    {
        struct winsize w{};
        if (isatty(STDOUT_FILENO) != 0 && ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0 && w.ws_col > 0)
        {
            m_rows = static_cast<int>(w.ws_row);
            m_cols = static_cast<int>(w.ws_col);
        }
        else
        {
            // Sensible fallback when not attached to a terminal or during headless runs
            m_rows = 24;
            m_cols = 80;
        }
    }

    auto Terminal::enable_bracketed_paste() const -> void
    {
        if (m_is_tty)
        {
            static_cast<void>(write_all(STDOUT_FILENO, "\033[?2004h"));
            std::fflush(stdout);
        }
    }

    auto Terminal::disable_bracketed_paste() const -> void
    {
        if (m_is_tty)
        {
            static_cast<void>(write_all(STDOUT_FILENO, "\033[?2004l"));
            std::fflush(stdout);
        }
    }

    auto Terminal::enable_kitty_keyboard() const -> void
    {
        if (m_is_tty)
        {
            // Push Kitty keyboard flags: 1 = DISAMBIGUATE_ESCAPE_CODES
            static_cast<void>(write_all(STDOUT_FILENO, "\033[>1u"));
            std::fflush(stdout);
        }
    }

    auto Terminal::disable_kitty_keyboard() const -> void
    {
        if (m_is_tty)
        {
            // Pop Kitty keyboard flags
            static_cast<void>(write_all(STDOUT_FILENO, "\033[<u"));
            std::fflush(stdout);
        }
    }

    auto Terminal::enable_mouse_capture() const -> void
    {
        if (m_is_tty)
        {
            // 1000h: standard click press/release reporting (no motion spam)
            // 1006h: SGR extended mode
            static_cast<void>(write_all(STDOUT_FILENO, "\033[?1000h\033[?1006h"));
            std::fflush(stdout);
        }
    }

    auto Terminal::disable_mouse_capture() const -> void
    {
        if (m_is_tty)
        {
            static_cast<void>(write_all(STDOUT_FILENO, "\033[?1006l\033[?1003l\033[?1002l\033[?1000l"));
            std::fflush(stdout);
        }
    }

    auto Terminal::enable_mouse_drag() const -> void
    {
        if (m_is_tty)
        {
            // 1002h: button event reporting (motion while button pressed)
            static_cast<void>(write_all(STDOUT_FILENO, "\033[?1002h"));
            std::fflush(stdout);
        }
    }

    auto Terminal::disable_mouse_drag() const -> void
    {
        if (m_is_tty)
        {
            static_cast<void>(write_all(STDOUT_FILENO, "\033[?1002l"));
            std::fflush(stdout);
        }
    }

    auto Terminal::enable_mouse_hover() const -> void
    {
        if (m_is_tty)
        {
            // 1003h: any-event mouse reporting (all motion including hover)
            static_cast<void>(write_all(STDOUT_FILENO, "\033[?1003h"));
            std::fflush(stdout);
        }
    }

    auto Terminal::disable_mouse_hover() const -> void
    {
        if (m_is_tty)
        {
            static_cast<void>(write_all(STDOUT_FILENO, "\033[?1003l"));
            std::fflush(stdout);
        }
    }

    Terminal::Terminal()
    {
        m_is_tty = (isatty(STDIN_FILENO) != 0 && isatty(STDOUT_FILENO) != 0);
        m_color_mode = detect_system_color_mode();
        set_dimensions();
        if (m_is_tty)
        {
            static_cast<void>(enter_raw_mode());
        }
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
