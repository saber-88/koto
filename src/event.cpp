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
    // Helper to decode XTerm / Kitty modifier parameter: 1 + flags
    // Bit 0: Shift (1)
    // Bit 1: Alt (2)
    // Bit 2: Ctrl (4)
    // Bit 3: Super / Meta (8)
    // Bit 5: Meta (32)
    static void decode_modifier(int mod_param, bool& shift, bool& alt, bool& ctrl, bool& meta)
    {
        if (mod_param >= 2)
        {
            int flags = mod_param - 1;
            shift = (flags & 1) != 0;
            alt   = (flags & 2) != 0;
            ctrl  = (flags & 4) != 0;
            meta  = (flags & 8) != 0 || (flags & 32) != 0;
        }
    }

    static std::optional<MouseEvent> parse_sgr_mouse(std::string_view sv)
    {
        // Must start with "\033[<" and end with 'M' or 'm'
        if (sv.size() < 6 || sv[0] != '\033' || sv[1] != '[' || sv[2] != '<')
        {
            return std::nullopt;
        }
        char term = sv.back();
        if (term != 'M' && term != 'm')
        {
            return std::nullopt;
        }

        size_t i = 3;
        int btn = 0;
        while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9')
        {
            btn = btn * 10 + (sv[i] - '0');
            i++;
        }
        if (i >= sv.size() || sv[i] != ';') return std::nullopt;
        i++;

        int col = 0;
        while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9')
        {
            col = col * 10 + (sv[i] - '0');
            i++;
        }
        if (i >= sv.size() || sv[i] != ';') return std::nullopt;
        i++;

        int row = 0;
        while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9')
        {
            row = row * 10 + (sv[i] - '0');
            i++;
        }

        MouseButton button = MouseButton::None;
        if (btn == 64) button = MouseButton::WheelUp;
        else if (btn == 65) button = MouseButton::WheelDown;
        else
        {
            int raw_btn = btn & 3;
            if (raw_btn == 0) button = MouseButton::Left;
            else if (raw_btn == 1) button = MouseButton::Middle;
            else if (raw_btn == 2) button = MouseButton::Right;
        }

        MouseAction action = (term == 'm') ? MouseAction::Release : MouseAction::Press;
        if ((btn & 32) != 0 && term == 'M')
        {
            action = MouseAction::Move;
        }

        bool shift = (btn & 4) != 0;
        bool alt = (btn & 8) != 0;
        bool ctrl = (btn & 16) != 0;

        return MouseEvent{
            .row = std::max(0, row - 1),
            .col = std::max(0, col - 1),
            .button = button,
            .action = action,
            .shift = shift,
            .alt = alt,
            .ctrl = ctrl
        };
    }

    static std::optional<KeyEvent> parse_csi(std::string_view sv)
    {
        // Must start with "\033[" and have at least 3 characters
        if (sv.size() < 3 || sv[0] != '\033' || sv[1] != '[')
        {
            return std::nullopt;
        }

        // Shift+Tab: "\033[Z"
        if (sv.size() == 3 && sv[2] == 'Z')
        {
            return KeyEvent{static_cast<uint32_t>(Key::BackTab), false, true, false, false};
        }

        // Standard unshifted arrows / home / end: "\033[A", "\033[B", "\033[C", "\033[D", "\033[H", "\033[F"
        if (sv.size() == 3)
        {
            switch (sv[2])
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

        // Terminator is the last byte in the CSI sequence
        char term = sv.back();
        size_t len = sv.size();

        // Parse parameters between sv[2] and sv[len - 1]
        size_t i = 2;
        int p1 = 0;
        bool has_p1 = false;
        while (i < len - 1 && sv[i] >= '0' && sv[i] <= '9')
        {
            p1 = p1 * 10 + (sv[i] - '0');
            has_p1 = true;
            i++;
        }

        // Skip any sub-parameters in p1 (e.g. p1:sub)
        while (i < len - 1 && sv[i] != ';')
        {
            i++;
        }

        int p2 = 0;
        bool has_p2 = false;
        int event_type = 1; // 1 = press, 2 = repeat, 3 = release

        if (i < len - 1 && sv[i] == ';')
        {
            i++;
            while (i < len - 1 && sv[i] >= '0' && sv[i] <= '9')
            {
                p2 = p2 * 10 + (sv[i] - '0');
                has_p2 = true;
                i++;
            }

            // Check for sub-parameters in p2 (e.g. modifier:event_type)
            if (i < len - 1 && sv[i] == ':')
            {
                i++;
                int ev = 0;
                while (i < len - 1 && sv[i] >= '0' && sv[i] <= '9')
                {
                    ev = ev * 10 + (sv[i] - '0');
                    i++;
                }
                if (ev > 0)
                {
                    event_type = ev;
                }
            }
        }

        // In Kitty keyboard protocol, event_type == 3 represents key release; ignore it
        if (event_type == 3)
        {
            return std::nullopt;
        }

        bool shift = false;
        bool alt = false;
        bool ctrl = false;
        bool meta = false;

        // Check if terminator is letter A-D, H, F, P-S: e.g., "\033[1;5A"
        if (term == 'A' || term == 'B' || term == 'C' || term == 'D' ||
            term == 'H' || term == 'F' || term == 'P' || term == 'Q' ||
            term == 'R' || term == 'S')
        {
            if (has_p2)
            {
                decode_modifier(p2, shift, alt, ctrl, meta);
            }
            else if (has_p1)
            {
                decode_modifier(p1, shift, alt, ctrl, meta);
            }

            switch (term)
            {
                case 'A': return KeyEvent{static_cast<uint32_t>(Key::Up), ctrl, shift, alt, meta};
                case 'B': return KeyEvent{static_cast<uint32_t>(Key::Down), ctrl, shift, alt, meta};
                case 'C': return KeyEvent{static_cast<uint32_t>(Key::Right), ctrl, shift, alt, meta};
                case 'D': return KeyEvent{static_cast<uint32_t>(Key::Left), ctrl, shift, alt, meta};
                case 'H': return KeyEvent{static_cast<uint32_t>(Key::Home), ctrl, shift, alt, meta};
                case 'F': return KeyEvent{static_cast<uint32_t>(Key::End), ctrl, shift, alt, meta};
                case 'P': return KeyEvent{static_cast<uint32_t>(Key::F1), ctrl, shift, alt, meta};
                case 'Q': return KeyEvent{static_cast<uint32_t>(Key::F2), ctrl, shift, alt, meta};
                case 'R': return KeyEvent{static_cast<uint32_t>(Key::F3), ctrl, shift, alt, meta};
                case 'S': return KeyEvent{static_cast<uint32_t>(Key::F4), ctrl, shift, alt, meta};
                default: break;
            }
        }

        // Tilde sequences: e.g., "\033[15~", "\033[15;5~", "\033[3~"
        if (term == '~' && has_p1)
        {
            if (has_p2)
            {
                decode_modifier(p2, shift, alt, ctrl, meta);
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
                return KeyEvent{static_cast<uint32_t>(k), ctrl, shift, alt, meta};
            }
        }

        // Kitty Keyboard Protocol (CSI u): "\033[<codepoint>;<mod>u" or "\033[<codepoint>u"
        if (term == 'u' && has_p1)
        {
            if (has_p2)
            {
                decode_modifier(p2, shift, alt, ctrl, meta);
            }

            uint32_t codepoint = static_cast<uint32_t>(p1);

            // Disambiguated control keys
            if (codepoint == 13 || codepoint == 57414) return KeyEvent{static_cast<uint32_t>(Key::Enter), ctrl, shift, alt, meta};
            if (codepoint == 9)                       return KeyEvent{static_cast<uint32_t>(Key::Tab), ctrl, shift, alt, meta};
            if (codepoint == 27)                      return KeyEvent{static_cast<uint32_t>(Key::Escape), ctrl, shift, alt, meta};
            if (codepoint == 127)                     return KeyEvent{static_cast<uint32_t>(Key::Backspace), ctrl, shift, alt, meta};

            // Kitty functional key mappings (Private Use Area)
            if (codepoint == 57417) return KeyEvent{static_cast<uint32_t>(Key::Left), ctrl, shift, alt, meta};
            if (codepoint == 57418) return KeyEvent{static_cast<uint32_t>(Key::Right), ctrl, shift, alt, meta};
            if (codepoint == 57419) return KeyEvent{static_cast<uint32_t>(Key::Up), ctrl, shift, alt, meta};
            if (codepoint == 57420) return KeyEvent{static_cast<uint32_t>(Key::Down), ctrl, shift, alt, meta};
            if (codepoint == 57421) return KeyEvent{static_cast<uint32_t>(Key::PageUp), ctrl, shift, alt, meta};
            if (codepoint == 57422) return KeyEvent{static_cast<uint32_t>(Key::PageDown), ctrl, shift, alt, meta};
            if (codepoint == 57423) return KeyEvent{static_cast<uint32_t>(Key::Home), ctrl, shift, alt, meta};
            if (codepoint == 57424) return KeyEvent{static_cast<uint32_t>(Key::End), ctrl, shift, alt, meta};
            if (codepoint == 57425) return KeyEvent{static_cast<uint32_t>(Key::Insert), ctrl, shift, alt, meta};
            if (codepoint == 57426) return KeyEvent{static_cast<uint32_t>(Key::Delete), ctrl, shift, alt, meta};

            // Ignore standalone lock/modifier key events
            if ((codepoint >= 57358 && codepoint <= 57363) || (codepoint >= 57441 && codepoint <= 57454))
            {
                return std::nullopt;
            }

            // Normalize Ctrl+letter to uppercase to match standard terminal conventions
            if (ctrl && codepoint >= 'a' && codepoint <= 'z')
            {
                codepoint = codepoint - 'a' + 'A';
            }
            else if (shift && !ctrl && codepoint >= 'a' && codepoint <= 'z')
            {
                codepoint = codepoint - 'a' + 'A';
            }

            return KeyEvent{codepoint, ctrl, shift, alt, meta};
        }

        return std::nullopt;
    }

    static std::optional<KeyEvent> parse_ss3(std::string_view sv)
    {
        // Must start with "\033O"
        if (sv.size() < 3 || sv[0] != '\033' || sv[1] != 'O')
        {
            return std::nullopt;
        }

        switch (sv[2])
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

    static auto parse_event_internal(std::string_view& in) -> Event
    {
        if (in.empty())
        {
            return TickEvent{};
        }

        // Bracketed Paste Mode: "\033[200~" ... "\033[201~"
        if (in.starts_with("\033[200~"))
        {
            size_t end_idx = in.find("\033[201~");
            if (end_idx != std::string_view::npos)
            {
                std::string_view raw = in.substr(6, end_idx - 6);
                in.remove_prefix(end_idx + 6);

                // Normalize CRLF and CR to LF
                std::string text;
                text.reserve(raw.size());
                for (size_t i = 0; i < raw.size(); ++i)
                {
                    if (raw[i] == '\r')
                    {
                        if (i + 1 < raw.size() && raw[i + 1] == '\n')
                        {
                            continue; // skip '\r' before '\n'
                        }
                        text.push_back('\n');
                    }
                    else
                    {
                        text.push_back(raw[i]);
                    }
                }
                return PasteEvent{std::move(text)};
            }
            // Terminator hasn't arrived yet
            return TickEvent{};
        }

        // Escape sequences starting with \033
        if (in[0] == '\033')
        {
            // Standalone Escape
            if (in.size() == 1)
            {
                in.remove_prefix(1);
                return KeyEvent{static_cast<uint32_t>(Key::Escape)};
            }

            // CSI sequence: "\033[" ... [0x40 - 0x7e]
            if (in[1] == '[')
            {
                // SGR mouse sequence: "\033[<" ... ['M' or 'm']
                if (in.size() >= 3 && in[2] == '<')
                {
                    size_t i = 3;
                    while (i < in.size() && in[i] != 'M' && in[i] != 'm')
                    {
                        i++;
                    }
                    if (i < in.size())
                    {
                        size_t mouse_len = i + 1;
                        std::string_view mouse_str = in.substr(0, mouse_len);
                        in.remove_prefix(mouse_len);
                        if (auto me = parse_sgr_mouse(mouse_str))
                        {
                            return *me;
                        }
                        return TickEvent{};
                    }
                    // Incomplete mouse sequence in buffer
                    return TickEvent{};
                }

                size_t i = 2;
                while (i < in.size() &&
                       (static_cast<unsigned char>(in[i]) < 0x40 ||
                        static_cast<unsigned char>(in[i]) > 0x7e))
                {
                    i++;
                }

                if (i < in.size())
                {
                    size_t csi_len = i + 1;
                    std::string_view csi_str = in.substr(0, csi_len);
                    in.remove_prefix(csi_len);
                    if (auto ke = parse_csi(csi_str))
                    {
                        return *ke;
                    }
                    return TickEvent{};
                }
                // Incomplete CSI sequence in buffer
                return TickEvent{};
            }

            // SS3 sequence: "\033O" + char
            if (in[1] == 'O')
            {
                if (in.size() >= 3)
                {
                    std::string_view ss3_str = in.substr(0, 3);
                    in.remove_prefix(3);
                    if (auto ke = parse_ss3(ss3_str))
                    {
                        return *ke;
                    }
                    return TickEvent{};
                }
                return TickEvent{};
            }

            // Alt + single-byte ASCII
            if (static_cast<unsigned char>(in[1]) < 0x80)
            {
                char c = in[1];
                in.remove_prefix(2);

                if (c == '\r' || c == '\n') return KeyEvent{static_cast<uint32_t>(Key::Enter), false, false, true, false};
                if (c == '\t')             return KeyEvent{static_cast<uint32_t>(Key::Tab), false, false, true, false};
                if (c == 127 || c == 8)    return KeyEvent{static_cast<uint32_t>(Key::Backspace), false, false, true, false};
                if (c == '\033')           return KeyEvent{static_cast<uint32_t>(Key::Escape), false, false, true, false};
                if (c >= 'A' && c <= 'Z')
                {
                    return KeyEvent{static_cast<uint32_t>(c), false, true, true, false};
                }
                if (c >= 32)
                {
                    return KeyEvent{static_cast<uint32_t>(c), false, false, true, false};
                }
                return TickEvent{};
            }

            // Alt + multi-byte UTF-8
            std::string_view alt_body = in.substr(1);
            size_t before_len = alt_body.size();
            char32_t cp = next_codepoint(alt_body);
            if (cp != 0)
            {
                size_t utf8_len = before_len - alt_body.size();
                in.remove_prefix(1 + utf8_len);
                return KeyEvent{static_cast<uint32_t>(cp), false, false, true, false};
            }

            in.remove_prefix(1);
            return TickEvent{};
        }

        // Tab
        if (in[0] == '\t')
        {
            in.remove_prefix(1);
            return KeyEvent{static_cast<uint32_t>(Key::Tab)};
        }

        // Enter: \r\n, \r, \n
        if (in[0] == '\r' || in[0] == '\n')
        {
            if (in.size() >= 2 && in[0] == '\r' && in[1] == '\n')
            {
                in.remove_prefix(2);
            }
            else
            {
                in.remove_prefix(1);
            }
            return KeyEvent{static_cast<uint32_t>(Key::Enter)};
        }

        // Backspace
        if (in[0] == 127 || in[0] == 8)
        {
            in.remove_prefix(1);
            return KeyEvent{static_cast<uint32_t>(Key::Backspace)};
        }

        // Ctrl+Space
        if (in[0] == 0)
        {
            in.remove_prefix(1);
            return KeyEvent{static_cast<uint32_t>(' '), true, false, false, false};
        }

        // Ctrl+A through Ctrl+Z (1..26)
        if (static_cast<unsigned char>(in[0]) >= 1 && static_cast<unsigned char>(in[0]) <= 26)
        {
            char c = in[0];
            in.remove_prefix(1);
            return KeyEvent{static_cast<uint32_t>('A' + c - 1), true, false, false, false};
        }

        // Regular printable ASCII
        if (static_cast<unsigned char>(in[0]) >= 32 && static_cast<unsigned char>(in[0]) < 127)
        {
            char c = in[0];
            in.remove_prefix(1);
            bool shift = (c >= 'A' && c <= 'Z');
            return KeyEvent{static_cast<uint32_t>(c), false, shift, false, false};
        }

        // Multi-byte UTF-8
        char32_t cp = next_codepoint(in);
        if (cp != 0)
        {
            return KeyEvent{static_cast<uint32_t>(cp)};
        }

        // Unparseable byte fallback: skip 1 byte
        in.remove_prefix(1);
        return TickEvent{};
    }

    auto EventLoop::parse(std::string_view bytes) -> Event
    {
        std::string_view sv = bytes;
        return parse_event_internal(sv);
    }

    auto EventLoop::parse_stdin() -> Event
    {
        if (m_pending_input.empty())
        {
            return TickEvent{};
        }

        // If a solitary Escape arrives, wait up to 25ms to see if subsequent sequence bytes arrive
        if (m_pending_input.size() == 1 && m_pending_input[0] == '\033')
        {
            struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
            if (::poll(&pfd, 1, 25) > 0 && (pfd.revents & POLLIN))
            {
                char extra[256] = {};
                ssize_t extra_n = read(STDIN_FILENO, extra, sizeof(extra));
                if (extra_n > 0)
                {
                    m_pending_input.append(extra, static_cast<size_t>(extra_n));
                }
            }
        }

        // If bracketed paste has begun but hasn't received terminator yet, read until \033[201~
        if (m_pending_input.starts_with("\033[200~") && m_pending_input.find("\033[201~") == std::string::npos)
        {
            while (m_pending_input.find("\033[201~") == std::string::npos)
            {
                struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
                if (::poll(&pfd, 1, 50) <= 0 || !(pfd.revents & POLLIN))
                {
                    break;
                }
                char chunk[512] = {};
                ssize_t chunk_n = read(STDIN_FILENO, chunk, sizeof(chunk));
                if (chunk_n <= 0) break;
                m_pending_input.append(chunk, static_cast<size_t>(chunk_n));
            }
        }

        std::string_view sv = m_pending_input;
        Event ev = parse_event_internal(sv);
        size_t consumed = m_pending_input.size() - sv.size();
        if (consumed > 0)
        {
            m_pending_input.erase(0, consumed);
        }
        else
        {
            m_pending_input.erase(0, 1);
        }
        return ev;
    }

    auto EventLoop::poll(std::int32_t timeout_ms) -> Event
    {
        // 1. If there are pending unconsumed bytes from a previous read (e.g. mouse release or rapid typing),
        // parse and return immediately WITHOUT blocking on read() or polling!
        if (!m_pending_input.empty())
        {
            return parse_stdin();
        }

        // 2. Wait for stdin or SIGWINCH resize signal
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
            // Drain all available bytes currently in OS kernel stdin buffer
            int available = 0;
            if (ioctl(STDIN_FILENO, FIONREAD, &available) == 0 && available > 0)
            {
                char buf[1024];
                while (available > 0)
                {
                    size_t to_read = std::min(sizeof(buf), static_cast<size_t>(available));
                    ssize_t n = read(STDIN_FILENO, buf, to_read);
                    if (n <= 0) break;
                    m_pending_input.append(buf, static_cast<size_t>(n));
                    available -= static_cast<int>(n);
                }
            }
            else
            {
                char buf[512] = {};
                ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
                if (n > 0)
                {
                    m_pending_input.append(buf, static_cast<size_t>(n));
                }
            }

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
