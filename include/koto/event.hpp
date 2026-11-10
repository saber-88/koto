#pragma once

#include "koto/id.hpp"
#include <chrono>
#include <csignal>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <unistd.h>

namespace koto
{
    enum class Key : uint32_t {
        None = 0,

        // navigation (offset into 0x80000000 range to never collide with Unicode codepoints)
        Up = 0x80000000, Down, Left, Right,
        Home, End, PageUp, PageDown,

        // editing
        Enter, Backspace, Delete, Insert, Tab, BackTab,

        // system
        Escape,

        // function keys
        F1, F2, F3,  F4,
        F5, F6, F7,  F8,
        F9, F10, F11, F12,

        // shift + arrow
        ShiftUp, ShiftDown, ShiftLeft, ShiftRight,

        // ctrl + arrow
        CtrlUp, CtrlDown, CtrlLeft, CtrlRight,

        // alt + arrow
        AltUp, AltDown, AltLeft, AltRight,
    };

    struct KeyEvent
    {
        uint32_t ch = 0;
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
        bool meta = false;

        bool is(Key k) const {
            if (k == Key::BackTab) return (ch == static_cast<uint32_t>(Key::Tab) && shift) || (ch == static_cast<uint32_t>(Key::BackTab));
            if (k == Key::ShiftUp) return (ch == static_cast<uint32_t>(Key::Up) && shift) || (ch == static_cast<uint32_t>(Key::ShiftUp));
            if (k == Key::ShiftDown) return (ch == static_cast<uint32_t>(Key::Down) && shift) || (ch == static_cast<uint32_t>(Key::ShiftDown));
            if (k == Key::ShiftLeft) return (ch == static_cast<uint32_t>(Key::Left) && shift) || (ch == static_cast<uint32_t>(Key::ShiftLeft));
            if (k == Key::ShiftRight) return (ch == static_cast<uint32_t>(Key::Right) && shift) || (ch == static_cast<uint32_t>(Key::ShiftRight));
            if (k == Key::CtrlUp) return (ch == static_cast<uint32_t>(Key::Up) && ctrl) || (ch == static_cast<uint32_t>(Key::CtrlUp));
            if (k == Key::CtrlDown) return (ch == static_cast<uint32_t>(Key::Down) && ctrl) || (ch == static_cast<uint32_t>(Key::CtrlDown));
            if (k == Key::CtrlLeft) return (ch == static_cast<uint32_t>(Key::Left) && ctrl) || (ch == static_cast<uint32_t>(Key::CtrlLeft));
            if (k == Key::CtrlRight) return (ch == static_cast<uint32_t>(Key::Right) && ctrl) || (ch == static_cast<uint32_t>(Key::CtrlRight));
            if (k == Key::AltUp) return (ch == static_cast<uint32_t>(Key::Up) && alt) || (ch == static_cast<uint32_t>(Key::AltUp));
            if (k == Key::AltDown) return (ch == static_cast<uint32_t>(Key::Down) && alt) || (ch == static_cast<uint32_t>(Key::AltDown));
            if (k == Key::AltLeft) return (ch == static_cast<uint32_t>(Key::Left) && alt) || (ch == static_cast<uint32_t>(Key::AltLeft));
            if (k == Key::AltRight) return (ch == static_cast<uint32_t>(Key::Right) && alt) || (ch == static_cast<uint32_t>(Key::AltRight));

            // Plain arrow navigation: modifiers must be clean
            if (k == Key::Up || k == Key::Down || k == Key::Left || k == Key::Right) {
                return ch == static_cast<uint32_t>(k) && !shift && !ctrl && !alt;
            }

            return ch == static_cast<uint32_t>(k);
        }

        bool is_char() const { return (ch >= 32 && ch < 127) || (ch >= 160 && ch < 0x80000000); }
    };

    struct ResizeEvent
    {
        uint32_t rows;
        uint32_t cols;
    };

    struct TickEvent
    {};

    struct PasteEvent
    {
        std::string text;
    };

    enum class MouseButton : uint8_t
    {
        Left = 0,
        Middle = 1,
        Right = 2,
        WheelUp = 64,
        WheelDown = 65,
        WheelLeft = 66,
        WheelRight = 67,
        None = 255
    };

    enum class MouseAction : uint8_t
    {
        Press,
        Release,
        Move
    };

    struct MouseEvent
    {
        int row = 0; // 0-based row
        int col = 0; // 0-based col
        MouseButton button = MouseButton::Left;
        MouseAction action = MouseAction::Press;
        bool shift = false;
        bool alt = false;
        bool ctrl = false;
        Id target = Id::none();

        constexpr bool is_press() const noexcept { return action == MouseAction::Press; }
        constexpr bool is_release() const noexcept { return action == MouseAction::Release; }
        constexpr bool is_move() const noexcept { return action == MouseAction::Move; }
        constexpr bool is_left_press() const noexcept { return is_press() && button == MouseButton::Left; }
        constexpr bool is_right_press() const noexcept { return is_press() && button == MouseButton::Right; }
        constexpr bool is_wheel_up() const noexcept { return button == MouseButton::WheelUp; }
        constexpr bool is_wheel_down() const noexcept { return button == MouseButton::WheelDown; }
        constexpr bool is_wheel_left() const noexcept { return button == MouseButton::WheelLeft; }
        constexpr bool is_wheel_right() const noexcept { return button == MouseButton::WheelRight; }
    };

    struct Event : std::variant<KeyEvent, ResizeEvent, TickEvent, PasteEvent, MouseEvent>
    {
        using variant::variant;

        bool is_key() const { return std::holds_alternative<KeyEvent>(*this); }
        bool is_resize() const { return std::holds_alternative<ResizeEvent>(*this); }
        bool is_tick() const { return std::holds_alternative<TickEvent>(*this); }
        bool is_paste() const { return std::holds_alternative<PasteEvent>(*this); }
        bool is_mouse() const { return std::holds_alternative<MouseEvent>(*this); }

        const KeyEvent* as_key() const { return std::get_if<KeyEvent>(this); }
        const ResizeEvent* as_resize() const { return std::get_if<ResizeEvent>(this); }
        const PasteEvent* as_paste() const { return std::get_if<PasteEvent>(this); }
        const MouseEvent* as_mouse() const { return std::get_if<MouseEvent>(this); }

        const KeyEvent& key() const { return std::get<KeyEvent>(*this); }
        const ResizeEvent& resize() const { return std::get<ResizeEvent>(*this); }
        const PasteEvent& paste() const { return std::get<PasteEvent>(*this); }
        const MouseEvent& mouse() const { return std::get<MouseEvent>(*this); }
    };


    class EventLoop
    {
        public: 

            EventLoop();
            ~EventLoop();

            EventLoop(const EventLoop&) = delete;
            EventLoop& operator=(const EventLoop&) = delete;

            auto poll(std::int32_t ms = 16) -> Event;
            bool has_pending() const noexcept { return !m_pending_input.empty(); }
            bool has_pending_event() const noexcept;
            bool is_eof() const noexcept { return m_eof; }

            // Direct buffer parser (useful for testing or feeding synthetic input streams)
            static auto parse(std::string_view bytes) -> Event;
            static auto on_sigwinch(int) -> void;

            // Feed raw bytes into pending input buffer (for streaming input / tests)
            void feed(std::string_view bytes)
            {
                m_pending_input.append(bytes);
                if (m_pending_input.starts_with("\033[200~"))
                {
                    m_paste_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
                }
                else if (m_escape_deadline != std::chrono::steady_clock::time_point{})
                {
                    m_escape_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(35);
                }
            }

            // Thread-safe async wakeup: unblocks poll() from any background worker thread
            void wakeup() noexcept;

        private:

            static volatile sig_atomic_t m_pipe_write_fd;
            std::string m_pending_input;
            struct sigaction m_old_sa{};
            int m_pipe_fds[2] = {-1, -1};
            bool m_eof = false;
            mutable std::chrono::steady_clock::time_point m_escape_deadline{};
            mutable std::chrono::steady_clock::time_point m_paste_deadline{};

            auto on_resize() -> Event;
            auto parse_stdin() -> Event;
            auto flush_incomplete_escape() -> Event;
            auto flush_incomplete_paste() -> Event;

    };
}
