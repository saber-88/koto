#include "koto/event.hpp"
#include "koto/utf8.hpp"
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <poll.h>
#include <string>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

namespace koto
{
    volatile sig_atomic_t EventLoop::m_pipe_write_fd = -1;
    
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
            if (btn < 100000) btn = btn * 10 + (sv[i] - '0');
            i++;
        }
        if (i >= sv.size() || sv[i] != ';') return std::nullopt;
        i++;

        int col = 0;
        while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9')
        {
            if (col < 100000) col = col * 10 + (sv[i] - '0');
            i++;
        }
        if (i >= sv.size() || sv[i] != ';') return std::nullopt;
        i++;

        int row = 0;
        while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9')
        {
            if (row < 100000) row = row * 10 + (sv[i] - '0');
            i++;
        }

        bool shift  = (btn & 4) != 0;
        bool alt    = (btn & 8) != 0;
        bool ctrl   = (btn & 16) != 0;
        bool motion = (btn & 32) != 0;

        int base_btn = btn & ~(4 | 8 | 16 | 32);

        MouseButton button = MouseButton::None;
        if (base_btn == 64)      button = MouseButton::WheelUp;
        else if (base_btn == 65) button = MouseButton::WheelDown;
        else if (base_btn == 66) button = MouseButton::WheelLeft;
        else if (base_btn == 67) button = MouseButton::WheelRight;
        else if (base_btn == 0)  button = MouseButton::Left;
        else if (base_btn == 1)  button = MouseButton::Middle;
        else if (base_btn == 2)  button = MouseButton::Right;
        else                     button = MouseButton::None;

        MouseAction action = (term == 'm') ? MouseAction::Release : MouseAction::Press;
        if (motion && term == 'M')
        {
            action = MouseAction::Move;
        }

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
            if (p1 < 100000) p1 = p1 * 10 + (sv[i] - '0');
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
                if (p2 < 100000) p2 = p2 * 10 + (sv[i] - '0');
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
                    if (ev < 100000) ev = ev * 10 + (sv[i] - '0');
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

    enum class ParseStatus
    {
        Complete,
        Incomplete,
        Invalid
    };

    struct ParseResult
    {
        ParseStatus status = ParseStatus::Invalid;
        Event event = TickEvent{};
        size_t consumed = 0;
    };

    static auto parse_step(std::string_view in) -> ParseResult
    {
        if (in.empty())
        {
            return ParseResult{ParseStatus::Complete, TickEvent{}, 0};
        }

        // 1. Bracketed Paste Mode: "\033[200~" ... "\033[201~"
        if (in.starts_with("\033[200~"))
        {
            size_t end_idx = in.find("\033[201~");
            if (end_idx != std::string_view::npos)
            {
                std::string_view raw = in.substr(6, end_idx - 6);
                std::string text;
                text.reserve(raw.size());
                for (size_t i = 0; i < raw.size(); ++i)
                {
                    if (raw[i] == '\r')
                    {
                        if (i + 1 < raw.size() && raw[i + 1] == '\n')
                        {
                            continue;
                        }
                        text.push_back('\n');
                    }
                    else
                    {
                        text.push_back(raw[i]);
                    }
                }
                return ParseResult{ParseStatus::Complete, PasteEvent{std::move(text)}, end_idx + 6};
            }
            // Terminator hasn't arrived yet
            return ParseResult{ParseStatus::Incomplete, TickEvent{}, 0};
        }

        // 2. Escape sequences starting with \033
        if (in[0] == '\033')
        {
            // Standalone Escape prefix
            if (in.size() == 1)
            {
                return ParseResult{ParseStatus::Incomplete, TickEvent{}, 0};
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
                        if ((in[i] < '0' || in[i] > '9') && in[i] != ';')
                        {
                            return ParseResult{ParseStatus::Invalid, TickEvent{}, 1};
                        }
                        i++;
                    }
                    if (i < in.size())
                    {
                        size_t mouse_len = i + 1;
                        if (auto me = parse_sgr_mouse(in.substr(0, mouse_len)))
                        {
                            return ParseResult{ParseStatus::Complete, *me, mouse_len};
                        }
                        return ParseResult{ParseStatus::Invalid, TickEvent{}, mouse_len};
                    }
                    if (in.size() > 64)
                    {
                        return ParseResult{ParseStatus::Invalid, TickEvent{}, 1};
                    }
                    return ParseResult{ParseStatus::Incomplete, TickEvent{}, 0};
                }

                // General CSI: parameter bytes 0x20..0x3F, final byte 0x40..0x7E
                size_t i = 2;
                while (i < in.size() &&
                       static_cast<unsigned char>(in[i]) >= 0x20 &&
                       static_cast<unsigned char>(in[i]) <= 0x3F)
                {
                    i++;
                }

                if (i < in.size())
                {
                    unsigned char term = static_cast<unsigned char>(in[i]);
                    if (term >= 0x40 && term <= 0x7E)
                    {
                        size_t csi_len = i + 1;
                        if (in.substr(0, csi_len) == "\033[200~")
                        {
                            // Incomplete bracketed paste body
                            return ParseResult{ParseStatus::Incomplete, TickEvent{}, 0};
                        }
                        if (auto ke = parse_csi(in.substr(0, csi_len)))
                        {
                            return ParseResult{ParseStatus::Complete, *ke, csi_len};
                        }
                        return ParseResult{ParseStatus::Complete, TickEvent{}, csi_len};
                    }
                    return ParseResult{ParseStatus::Invalid, TickEvent{}, 1};
                }

                if (in.size() > 64)
                {
                    return ParseResult{ParseStatus::Invalid, TickEvent{}, 1};
                }
                return ParseResult{ParseStatus::Incomplete, TickEvent{}, 0};
            }

            // SS3 sequence: "\033O" + char
            if (in[1] == 'O')
            {
                if (in.size() < 3)
                {
                    return ParseResult{ParseStatus::Incomplete, TickEvent{}, 0};
                }
                if (auto ke = parse_ss3(in.substr(0, 3)))
                {
                    return ParseResult{ParseStatus::Complete, *ke, 3};
                }
                return ParseResult{ParseStatus::Complete, TickEvent{}, 3};
            }

            // OSC sequence: "\033]" ... BEL (0x07) or ST ("\033\\")
            if (in[1] == ']')
            {
                size_t bell = in.find('\007');
                size_t st = in.find("\033\\");
                size_t end_osc = std::min(bell, st);
                if (end_osc != std::string_view::npos)
                {
                    size_t osc_len = (end_osc == bell) ? bell + 1 : st + 2;
                    return ParseResult{ParseStatus::Complete, TickEvent{}, osc_len};
                }
                if (in.size() > 256)
                {
                    return ParseResult{ParseStatus::Invalid, TickEvent{}, 1};
                }
                return ParseResult{ParseStatus::Incomplete, TickEvent{}, 0};
            }

            // Alt + single-byte ASCII
            if (static_cast<unsigned char>(in[1]) < 0x80)
            {
                char c = in[1];
                if (c == '\r' || c == '\n') return ParseResult{ParseStatus::Complete, KeyEvent{static_cast<uint32_t>(Key::Enter), false, false, true, false}, 2};
                if (c == '\t')             return ParseResult{ParseStatus::Complete, KeyEvent{static_cast<uint32_t>(Key::Tab), false, false, true, false}, 2};
                if (c == 127 || c == 8)    return ParseResult{ParseStatus::Complete, KeyEvent{static_cast<uint32_t>(Key::Backspace), false, false, true, false}, 2};
                if (c == '\033')           return ParseResult{ParseStatus::Complete, KeyEvent{static_cast<uint32_t>(Key::Escape), false, false, true, false}, 2};
                if (c >= 'A' && c <= 'Z')  return ParseResult{ParseStatus::Complete, KeyEvent{static_cast<uint32_t>(c), false, true, true, false}, 2};
                if (c >= 32)               return ParseResult{ParseStatus::Complete, KeyEvent{static_cast<uint32_t>(c), false, false, true, false}, 2};
                return ParseResult{ParseStatus::Complete, TickEvent{}, 2};
            }

            // Alt + multi-byte UTF-8
            unsigned char lead = static_cast<unsigned char>(in[1]);
            size_t expected_utf8 = 1;
            if ((lead & 0xE0) == 0xC0) expected_utf8 = 2;
            else if ((lead & 0xF0) == 0xE0) expected_utf8 = 3;
            else if ((lead & 0xF8) == 0xF0) expected_utf8 = 4;
            else
            {
                return ParseResult{ParseStatus::Invalid, TickEvent{}, 1};
            }

            if (in.size() < 1 + expected_utf8)
            {
                return ParseResult{ParseStatus::Incomplete, TickEvent{}, 0};
            }

            std::string_view alt_body = in.substr(1);
            size_t before_len = alt_body.size();
            char32_t cp = next_codepoint(alt_body);
            if (cp != 0 && cp != U'\uFFFD')
            {
                size_t utf8_len = before_len - alt_body.size();
                return ParseResult{ParseStatus::Complete, KeyEvent{static_cast<uint32_t>(cp), false, false, true, false}, 1 + utf8_len};
            }

            return ParseResult{ParseStatus::Invalid, TickEvent{}, 1};
        }

        // 3. Tab
        if (in[0] == '\t')
        {
            return ParseResult{ParseStatus::Complete, KeyEvent{static_cast<uint32_t>(Key::Tab)}, 1};
        }

        // 4. Enter: \r\n, \r, \n
        if (in[0] == '\r' || in[0] == '\n')
        {
            if (in.size() >= 2 && in[0] == '\r' && in[1] == '\n')
            {
                return ParseResult{ParseStatus::Complete, KeyEvent{static_cast<uint32_t>(Key::Enter)}, 2};
            }
            return ParseResult{ParseStatus::Complete, KeyEvent{static_cast<uint32_t>(Key::Enter)}, 1};
        }

        // 5. Backspace
        if (in[0] == 127 || in[0] == 8)
        {
            return ParseResult{ParseStatus::Complete, KeyEvent{static_cast<uint32_t>(Key::Backspace)}, 1};
        }

        // 6. Ctrl+Space
        if (in[0] == 0)
        {
            return ParseResult{ParseStatus::Complete, KeyEvent{static_cast<uint32_t>(' '), true, false, false, false}, 1};
        }

        // 7. Ctrl+A through Ctrl+Z (1..26)
        if (static_cast<unsigned char>(in[0]) >= 1 && static_cast<unsigned char>(in[0]) <= 26)
        {
            char c = in[0];
            return ParseResult{ParseStatus::Complete, KeyEvent{static_cast<uint32_t>('A' + c - 1), true, false, false, false}, 1};
        }

        // 8. Regular printable ASCII
        if (static_cast<unsigned char>(in[0]) >= 32 && static_cast<unsigned char>(in[0]) < 127)
        {
            char c = in[0];
            bool shift = (c >= 'A' && c <= 'Z');
            return ParseResult{ParseStatus::Complete, KeyEvent{static_cast<uint32_t>(c), false, shift, false, false}, 1};
        }

        // 9. Multi-byte UTF-8
        unsigned char lead = static_cast<unsigned char>(in[0]);
        size_t expected_utf8 = 1;
        if ((lead & 0xE0) == 0xC0) expected_utf8 = 2;
        else if ((lead & 0xF0) == 0xE0) expected_utf8 = 3;
        else if ((lead & 0xF8) == 0xF0) expected_utf8 = 4;
        else
        {
            return ParseResult{ParseStatus::Invalid, TickEvent{}, 1};
        }

        if (in.size() < expected_utf8)
        {
            return ParseResult{ParseStatus::Incomplete, TickEvent{}, 0};
        }

        std::string_view sv = in;
        size_t before_len = sv.size();
        char32_t cp = next_codepoint(sv);
        if (cp != 0 && cp != U'\uFFFD')
        {
            size_t consumed = before_len - sv.size();
            return ParseResult{ParseStatus::Complete, KeyEvent{static_cast<uint32_t>(cp)}, consumed};
        }

        return ParseResult{ParseStatus::Invalid, TickEvent{}, 1};
    }

    auto EventLoop::parse(std::string_view bytes) -> Event
    {
        if (bytes.empty()) return TickEvent{};
        if (bytes.size() == 1 && bytes[0] == '\033')
        {
            return KeyEvent{static_cast<uint32_t>(Key::Escape)};
        }
        ParseResult res = parse_step(bytes);
        if (res.status == ParseStatus::Complete)
        {
            return res.event;
        }
        return TickEvent{};
    }

    bool EventLoop::has_pending_event() const noexcept
    {
        if (m_pending_input.empty())
        {
            return false;
        }

        ParseResult res = parse_step(m_pending_input);
        if (res.status == ParseStatus::Complete || res.status == ParseStatus::Invalid)
        {
            return true;
        }

        // Bracketed paste is an atomic multi-byte stream; do NOT abort on short escape timeout (R1 fix)
        if (m_pending_input.starts_with("\033[200~"))
        {
            return false;
        }

        // Short incomplete sequences (ESC disambiguation or partial UTF-8):
        if (m_escape_deadline == std::chrono::steady_clock::time_point{})
        {
            m_escape_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(35);
            return false;
        }
        if (std::chrono::steady_clock::now() >= m_escape_deadline)
        {
            return true;
        }

        return false;
    }

    auto EventLoop::flush_incomplete_escape() -> Event
    {
        m_escape_deadline = {};
        if (m_pending_input.empty())
        {
            return TickEvent{};
        }

        // Bracketed paste must never be torn apart by flush_incomplete_escape (R1 fix)
        if (m_pending_input.starts_with("\033[200~"))
        {
            return TickEvent{};
        }

        if (m_pending_input[0] != '\033')
        {
            // Truncated/stale non-ESC byte (e.g. orphaned UTF-8 lead byte, R5 fix)
            m_pending_input.erase(0, 1);
            return TickEvent{};
        }

        // 1. Lone Escape prefix ("\033")
        if (m_pending_input.size() == 1)
        {
            m_pending_input.clear();
            return KeyEvent{static_cast<uint32_t>(Key::Escape)};
        }

        // 2. Alt + '[' ("\033[")
        if (m_pending_input == "\033[")
        {
            m_pending_input.clear();
            return KeyEvent{static_cast<uint32_t>('['), false, false, true, false};
        }

        // 3. Alt + Shift + 'O' ("\033O")
        if (m_pending_input == "\033O")
        {
            m_pending_input.clear();
            return KeyEvent{static_cast<uint32_t>('O'), false, true, true, false};
        }

        // 4. Alt + ']' ("\033]")
        if (m_pending_input == "\033]")
        {
            m_pending_input.clear();
            return KeyEvent{static_cast<uint32_t>(']'), false, false, true, false};
        }

        // 5. Incomplete / unrecognized escape sequence: emit Escape, leave remaining bytes
        m_pending_input.erase(0, 1);
        return KeyEvent{static_cast<uint32_t>(Key::Escape)};
    }

    auto EventLoop::parse_stdin() -> Event
    {
        if (m_pending_input.empty())
        {
            return TickEvent{};
        }

        ParseResult res = parse_step(m_pending_input);

        if (res.status == ParseStatus::Complete)
        {
            m_escape_deadline = {};
            m_pending_input.erase(0, res.consumed);
            return res.event;
        }

        if (res.status == ParseStatus::Incomplete)
        {
            // Do NOT apply short timeout to bracketed paste (R1 fix)
            if (m_pending_input.starts_with("\033[200~"))
            {
                return TickEvent{};
            }

            // Establish or check 35ms deadline for short sequences
            if (m_escape_deadline == std::chrono::steady_clock::time_point{})
            {
                m_escape_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(35);
            }
            else if (std::chrono::steady_clock::now() >= m_escape_deadline)
            {
                return flush_incomplete_escape();
            }

            return TickEvent{};
        }

        // Invalid sequence / malformed byte: drop bad byte
        size_t to_drop = res.consumed > 0 ? res.consumed : 1;
        m_pending_input.erase(0, to_drop);
        if (m_pending_input.empty()) m_escape_deadline = {};
        return TickEvent{};
    }

    auto EventLoop::on_sigwinch([[maybe_unused]] int signum) -> void
    {
        int saved_errno = errno;
        if (m_pipe_write_fd >= 0)
        {
            char b = 'R';
            [[maybe_unused]] ssize_t written = write(static_cast<int>(m_pipe_write_fd), &b, 1);
        }
        errno = saved_errno;
    }

    auto EventLoop::wakeup() noexcept -> void
    {
        if (m_pipe_fds[1] >= 0)
        {
            char b = 'W';
            [[maybe_unused]] ssize_t written = ::write(m_pipe_fds[1], &b, 1);
        }
    }

    auto EventLoop::poll(std::int32_t timeout_ms) -> Event
    {
        // 1. If there are pending unconsumed bytes from a previous read:
        if (!m_pending_input.empty())
        {
            ParseResult res = parse_step(m_pending_input);
            if (res.status == ParseStatus::Complete)
            {
                m_escape_deadline = {};
                m_pending_input.erase(0, res.consumed);
                return res.event;
            }
            if (res.status == ParseStatus::Invalid)
            {
                size_t to_drop = res.consumed > 0 ? res.consumed : 1;
                m_pending_input.erase(0, to_drop);
                if (m_pending_input.empty()) m_escape_deadline = {};
                return poll(0);
            }
            // ParseStatus::Incomplete:
            if (!m_pending_input.starts_with("\033[200~"))
            {
                if (m_escape_deadline == std::chrono::steady_clock::time_point{})
                {
                    m_escape_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(35);
                }
                else if (std::chrono::steady_clock::now() >= m_escape_deadline)
                {
                    return flush_incomplete_escape();
                }
            }
        }
        else
        {
            m_escape_deadline = {};
        }

        // 2. Wait for stdin or self-pipe (SIGWINCH resize / async wakeup)
        struct pollfd pfds[2] =
        {
            {m_eof ? -1 : STDIN_FILENO, POLLIN, 0},
            {m_pipe_fds[0],             POLLIN, 0},
        };

        int eff_timeout = timeout_ms;
        if (!m_pending_input.starts_with("\033[200~") && m_escape_deadline != std::chrono::steady_clock::time_point{})
        {
            auto now = std::chrono::steady_clock::now();
            if (now >= m_escape_deadline)
            {
                return flush_incomplete_escape();
            }
            auto rem_ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_escape_deadline - now).count();
            int esc_rem = static_cast<int>(std::max<int64_t>(1, rem_ms));
            if (eff_timeout < 0 || eff_timeout > esc_rem)
            {
                eff_timeout = esc_rem;
            }
        }

        int ret = ::poll(pfds, 2, eff_timeout);

        if (ret <= 0)
        {
            if (!m_pending_input.starts_with("\033[200~") && m_escape_deadline != std::chrono::steady_clock::time_point{})
            {
                if (std::chrono::steady_clock::now() >= m_escape_deadline)
                {
                    return flush_incomplete_escape();
                }
            }
            return TickEvent{};
        }

        if (m_pipe_fds[0] >= 0 && (pfds[1].revents & POLLIN))
        {
            // Drain self-pipe and check for resize signal or thread wakeup
            char buf[64];
            ssize_t bytes = read(m_pipe_fds[0], buf, sizeof(buf));
            bool got_resize = false;
            if (bytes > 0)
            {
                for (ssize_t i = 0; i < bytes; ++i)
                {
                    if (buf[static_cast<size_t>(i)] == 'R')
                    {
                        got_resize = true;
                        break;
                    }
                }
            }
            if (got_resize)
            {
                return on_resize();
            }
            return TickEvent{};
        }

        if (!m_eof && (pfds[0].revents & (POLLIN | POLLHUP | POLLERR)))
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
                    if (n <= 0)
                    {
                        if (n == 0 || errno == EIO || errno == EBADF) m_eof = true;
                        break;
                    }
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
                else if (n == 0 || errno == EIO || errno == EBADF)
                {
                    m_eof = true;
                }
            }

            // Reset deadline on new incoming bytes (R1 fix)
            if (m_escape_deadline != std::chrono::steady_clock::time_point{})
            {
                m_escape_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(35);
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
        if (pipe(m_pipe_fds) == 0)
        {
            for (int fd : m_pipe_fds)
            {
                int fl = fcntl(fd, F_GETFL, 0);
                if (fl >= 0)
                {
                    fcntl(fd, F_SETFL, fl | O_NONBLOCK);
                }
                int fdfl = fcntl(fd, F_GETFD, 0);
                if (fdfl >= 0)
                {
                    fcntl(fd, F_SETFD, fdfl | FD_CLOEXEC);
                }
            }
            m_pipe_write_fd = m_pipe_fds[1];
            struct sigaction sa{};
            sa.sa_handler = on_sigwinch;
            sa.sa_flags = SA_RESTART;
            sigaction(SIGWINCH, &sa, &m_old_sa);
        }
    }

    EventLoop::~EventLoop()
    {
        m_pipe_write_fd = -1;
        if (m_pipe_fds[0] >= 0)
        {
            close(m_pipe_fds[0]);
            m_pipe_fds[0] = -1;
        }
        if (m_pipe_fds[1] >= 0)
        {
            close(m_pipe_fds[1]);
            m_pipe_fds[1] = -1;
        }
        sigaction(SIGWINCH, &m_old_sa, nullptr);
    }
}
