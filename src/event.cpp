#include "koto/event.hpp"
#include "koto/utf8.hpp"
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <unistd.h>

namespace koto
{
    // Helper to decode XTerm modifier parameter: 1 + flags
    // 2=Shift, 3=Alt, 4=Shift+Alt, 5=Ctrl, 6=Ctrl+Shift, 7=Ctrl+Alt, 8=Ctrl+Alt+Shift
    static void decode_modifier(int mod_param, bool& shift, bool& alt, bool& ctrl)
    {
        if (mod_param >= 2)
        {
            int flags = mod_param - 1;
            shift = (flags & 1) != 0;
            alt   = (flags & 2) != 0;
            ctrl  = (flags & 4) != 0;
        }
    }

    static std::optional<KeyEvent> parse_csi(const char* buf, size_t len)
    {
        // Must start with "\033["
        if (len < 3 || buf[0] != '\033' || buf[1] != '[')
        {
            return std::nullopt;
        }

        // Shift+Tab: "\033[Z"
        if (len == 3 && buf[2] == 'Z')
        {
            return KeyEvent{static_cast<uint32_t>(Key::BackTab), false, true, false};
        }

        // Standard unshifted arrows / home / end: "\033[A", "\033[B", "\033[C", "\033[D", "\033[H", "\033[F"
        if (len == 3)
        {
            switch (buf[2])
            {
                case 'A': return KeyEvent{static_cast<uint32_t>(Key::Up)};
                case 'B': return KeyEvent{static_cast<uint32_t>(Key::Down)};
                case 'C': return KeyEvent{static_cast<uint32_t>(Key::Right)};
                case 'D': return KeyEvent{static_cast<uint32_t>(Key::Left)};
                case 'H': return KeyEvent{static_cast<uint32_t>(Key::Home)};
                case 'F': return KeyEvent{static_cast<uint32_t>(Key::End)};
                default: break;
            }
        }

        // Parameterized CSI: e.g. "\033[1;5A", "\033[15~", "\033[15;5~", "\033[13;2u"
        size_t i = 2;
        int p1 = 0;
        bool has_p1 = false;
        while (i < len && buf[i] >= '0' && buf[i] <= '9')
        {
            p1 = p1 * 10 + (buf[i] - '0');
            has_p1 = true;
            i++;
        }

        int p2 = 0;
        bool has_p2 = false;
        if (i < len && buf[i] == ';')
        {
            i++;
            while (i < len && buf[i] >= '0' && buf[i] <= '9')
            {
                p2 = p2 * 10 + (buf[i] - '0');
                has_p2 = true;
                i++;
            }
        }

        if (i >= len) return std::nullopt;
        char term = buf[i];

        bool shift = false;
        bool alt = false;
        bool ctrl = false;

        // Check if terminator is letter A-D, H, F, P-S: e.g., "\033[1;5A"
        if (term == 'A' || term == 'B' || term == 'C' || term == 'D' ||
            term == 'H' || term == 'F' || term == 'P' || term == 'Q' ||
            term == 'R' || term == 'S')
        {
            if (has_p2)
            {
                decode_modifier(p2, shift, alt, ctrl);
            }
            else if (has_p1)
            {
                decode_modifier(p1, shift, alt, ctrl);
            }

            switch (term)
            {
                case 'A': return KeyEvent{static_cast<uint32_t>(Key::Up), ctrl, shift, alt};
                case 'B': return KeyEvent{static_cast<uint32_t>(Key::Down), ctrl, shift, alt};
                case 'C': return KeyEvent{static_cast<uint32_t>(Key::Right), ctrl, shift, alt};
                case 'D': return KeyEvent{static_cast<uint32_t>(Key::Left), ctrl, shift, alt};
                case 'H': return KeyEvent{static_cast<uint32_t>(Key::Home), ctrl, shift, alt};
                case 'F': return KeyEvent{static_cast<uint32_t>(Key::End), ctrl, shift, alt};
                case 'P': return KeyEvent{static_cast<uint32_t>(Key::F1), ctrl, shift, alt};
                case 'Q': return KeyEvent{static_cast<uint32_t>(Key::F2), ctrl, shift, alt};
                case 'R': return KeyEvent{static_cast<uint32_t>(Key::F3), ctrl, shift, alt};
                case 'S': return KeyEvent{static_cast<uint32_t>(Key::F4), ctrl, shift, alt};
                default: break;
            }
        }

        // Tilde sequences: e.g., "\033[15~", "\033[15;5~", "\033[3~"
        if (term == '~' && has_p1)
        {
            if (has_p2)
            {
                decode_modifier(p2, shift, alt, ctrl);
            }

            Key k = Key::None;
            switch (p1)
            {
                case 1:  case 7:  k = Key::Home; break;
                case 2:           k = Key::Insert; break;
                case 3:           k = Key::Delete; break;
                case 4:  case 8:  k = Key::End; break;
                case 5:           k = Key::PageUp; break;
                case 6:           k = Key::PageDown; break;
                case 11:          k = Key::F1; break;
                case 12:          k = Key::F2; break;
                case 13:          k = Key::F3; break;
                case 14:          k = Key::F4; break;
                case 15:          k = Key::F5; break;
                case 17:          k = Key::F6; break;
                case 18:          k = Key::F7; break;
                case 19:          k = Key::F8; break;
                case 20:          k = Key::F9; break;
                case 21:          k = Key::F10; break;
                case 23:          k = Key::F11; break;
                case 24:          k = Key::F12; break;
                default: break;
            }
            if (k != Key::None)
            {
                return KeyEvent{static_cast<uint32_t>(k), ctrl, shift, alt};
            }
        }

        // CSI u keyboard protocol (Kitty / WezTerm / Ghostty / foot): "\033[<codepoint>;<mod>u"
        if (term == 'u' && has_p1)
        {
            if (has_p2)
            {
                decode_modifier(p2, shift, alt, ctrl);
            }
            uint32_t codepoint = static_cast<uint32_t>(p1);
            if (codepoint == 13) return KeyEvent{static_cast<uint32_t>(Key::Enter), ctrl, shift, alt};
            if (codepoint == 9)  return KeyEvent{static_cast<uint32_t>(Key::Tab), ctrl, shift, alt};
            if (codepoint == 27) return KeyEvent{static_cast<uint32_t>(Key::Escape), ctrl, shift, alt};
            if (codepoint == 127) return KeyEvent{static_cast<uint32_t>(Key::Backspace), ctrl, shift, alt};
            return KeyEvent{codepoint, ctrl, shift, alt};
        }

        return std::nullopt;
    }

    static std::optional<KeyEvent> parse_ss3(const char* buf, size_t len)
    {
        // Must start with "\033O"
        if (len < 3 || buf[0] != '\033' || buf[1] != 'O')
        {
            return std::nullopt;
        }

        switch (buf[2])
        {
            case 'A': return KeyEvent{static_cast<uint32_t>(Key::Up)};
            case 'B': return KeyEvent{static_cast<uint32_t>(Key::Down)};
            case 'C': return KeyEvent{static_cast<uint32_t>(Key::Right)};
            case 'D': return KeyEvent{static_cast<uint32_t>(Key::Left)};
            case 'H': return KeyEvent{static_cast<uint32_t>(Key::Home)};
            case 'F': return KeyEvent{static_cast<uint32_t>(Key::End)};
            case 'P': return KeyEvent{static_cast<uint32_t>(Key::F1)};
            case 'Q': return KeyEvent{static_cast<uint32_t>(Key::F2)};
            case 'R': return KeyEvent{static_cast<uint32_t>(Key::F3)};
            case 'S': return KeyEvent{static_cast<uint32_t>(Key::F4)};
            default: break;
        }
        return std::nullopt;
    }

    auto EventLoop::parse_escape(const char *buf, ssize_t n) -> Event
    {
        size_t len = static_cast<size_t>(n);
        if (len >= 2 && buf[1] == '[')
        {
            if (auto ke = parse_csi(buf, len))
            {
                return *ke;
            }
        }
        else if (len >= 2 && buf[1] == 'O')
        {
            if (auto ke = parse_ss3(buf, len))
            {
                return *ke;
            }
        }

        // Alt + key (single byte after \033)
        if (len == 2)
        {
            char c = buf[1];
            if (c == '\r' || c == '\n') return KeyEvent{static_cast<uint32_t>(Key::Enter), false, false, true};
            if (c == '\t') return KeyEvent{static_cast<uint32_t>(Key::Tab), false, false, true};
            if (c == 127 || c == 8) return KeyEvent{static_cast<uint32_t>(Key::Backspace), false, false, true};
            if (c == '\033') return KeyEvent{static_cast<uint32_t>(Key::Escape), false, false, true};
            if (c >= 'A' && c <= 'Z')
            {
                return KeyEvent{static_cast<uint32_t>(c), false, true, true};
            }
            if (c >= 32)
            {
                return KeyEvent{static_cast<uint32_t>(c), false, false, true};
            }
        }

        // Alt + multi-byte UTF-8 character
        if (len > 2 && buf[1] != '[' && buf[1] != 'O')
        {
            const char* ptr = buf + 1;
            const char* end = buf + static_cast<size_t>(n);
            char32_t cp = next_codepoint(ptr, end);
            if (cp != 0)
            {
                return KeyEvent{static_cast<uint32_t>(cp), false, false, true};
            }
        }

        return TickEvent{};
    }

    auto EventLoop::parse_stdin() -> Event
    {
        char buf[64] = {};
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));

        if (n <= 0)
        {
            return TickEvent{};
        }

        // If a solitary Escape arrives, give a tiny 25ms window for following
        // escape sequence bytes (handles network/SSH latency and slow terminals)
        if (n == 1 && buf[0] == '\033')
        {
            struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
            if (::poll(&pfd, 1, 25) > 0 && (pfd.revents & POLLIN))
            {
                ssize_t extra = read(STDIN_FILENO, buf + 1, sizeof(buf) - 1);
                if (extra > 0)
                {
                    n += extra;
                }
            }
            if (n == 1)
            {
                return KeyEvent{static_cast<uint32_t>(Key::Escape)};
            }
        }

        if (n > 1 && buf[0] == '\033')
        {
            Event esc_ev = parse_escape(buf, n);
            if (std::holds_alternative<KeyEvent>(esc_ev))
            {
                return esc_ev;
            }
            return TickEvent{};
        }

        // Tab
        if (n == 1 && buf[0] == '\t')
        {
            return KeyEvent{static_cast<uint32_t>(Key::Tab)};
        }

        // Enter
        if (n == 1 && (buf[0] == '\r' || buf[0] == '\n'))
        {
            return KeyEvent{static_cast<uint32_t>(Key::Enter)};
        }

        // Backspace
        if (n == 1 && (buf[0] == 127 || buf[0] == 8))
        {
            return KeyEvent{static_cast<uint32_t>(Key::Backspace)};
        }

        // Ctrl+Space
        if (n == 1 && buf[0] == 0)
        {
            return KeyEvent{static_cast<uint32_t>(' '), true, false, false};
        }

        // Ctrl+A through Ctrl+Z (1..26, excluding 9 (\t), 10 (\n), 13 (\r))
        if (n == 1 && buf[0] >= 1 && buf[0] <= 26)
        {
            return KeyEvent{static_cast<uint32_t>('A' + buf[0] - 1), true, false, false};
        }

        // Regular printable ASCII character
        if (n == 1 && buf[0] >= 32 && buf[0] < 127)
        {
            bool shift = (buf[0] >= 'A' && buf[0] <= 'Z');
            return KeyEvent{static_cast<uint32_t>(buf[0]), false, shift, false};
        }

        // Multi-byte UTF-8 character (typed Unicode)
        if (n > 1 && buf[0] != '\033')
        {
            const char* ptr = buf;
            const char* end = buf + static_cast<size_t>(n);
            char32_t cp = next_codepoint(ptr, end);
            if (cp != 0)
            {
                return KeyEvent{static_cast<uint32_t>(cp)};
            }
        }

        return TickEvent{};
    }

    auto EventLoop::poll(std::int32_t timeout_ms) -> Event
    {
        struct pollfd pfds[2] =
        {
            {STDIN_FILENO,  POLLIN, 0},
            {m_signal_fd,   POLLIN, 0},
        };

        int ret = ::poll(pfds, 2, timeout_ms);

        if (ret <= 0)
        {
            return TickEvent{};
        }

        if (pfds[1].revents & POLLIN)
        {
            // resize
            struct signalfd_siginfo info;
            ssize_t bytes = read(m_signal_fd, &info, sizeof(info));
            static_cast<void>(bytes);
            return on_resize();
        }

        if (pfds[0].revents & POLLIN)
        {
            // key
            return parse_stdin();
        }

        return TickEvent{};
    }

    auto EventLoop::on_resize() -> Event
    {
        struct winsize w;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        {
            uint32_t r = w.ws_row > 0 ? static_cast<uint32_t>(w.ws_row) : 1u;
            uint32_t c = w.ws_col > 0 ? static_cast<uint32_t>(w.ws_col) : 1u;
            return ResizeEvent{r, c};
        }
        return ResizeEvent{24u, 80u};
    }

    EventLoop::EventLoop()
    {   
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGWINCH);
        sigprocmask(SIG_BLOCK, &mask, &m_old_mask);
        m_signal_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    }

    EventLoop::~EventLoop()
    {
        if (m_signal_fd >= 0)
        {
            close(m_signal_fd);
        }
        // restore original signal mask
        sigprocmask(SIG_SETMASK, &m_old_mask, nullptr);
    }
}