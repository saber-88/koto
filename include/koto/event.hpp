#pragma once

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
            if (ch == static_cast<uint32_t>(k)) return true;
            if (k == Key::BackTab) return (ch == static_cast<uint32_t>(Key::Tab) && shift) || (ch == static_cast<uint32_t>(Key::BackTab));
            if (k == Key::ShiftUp) return ch == static_cast<uint32_t>(Key::Up) && shift;
            if (k == Key::ShiftDown) return ch == static_cast<uint32_t>(Key::Down) && shift;
            if (k == Key::ShiftLeft) return ch == static_cast<uint32_t>(Key::Left) && shift;
            if (k == Key::ShiftRight) return ch == static_cast<uint32_t>(Key::Right) && shift;
            if (k == Key::CtrlUp) return ch == static_cast<uint32_t>(Key::Up) && ctrl;
            if (k == Key::CtrlDown) return ch == static_cast<uint32_t>(Key::Down) && ctrl;
            if (k == Key::CtrlLeft) return ch == static_cast<uint32_t>(Key::Left) && ctrl;
            if (k == Key::CtrlRight) return ch == static_cast<uint32_t>(Key::Right) && ctrl;
            if (k == Key::AltUp) return ch == static_cast<uint32_t>(Key::Up) && alt;
            if (k == Key::AltDown) return ch == static_cast<uint32_t>(Key::Down) && alt;
            if (k == Key::AltLeft) return ch == static_cast<uint32_t>(Key::Left) && alt;
            if (k == Key::AltRight) return ch == static_cast<uint32_t>(Key::Right) && alt;
            return false;
        }

        bool is_char() const { return ch < 0x80000000 && ch >= 32; }
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

            // Direct buffer parser (useful for testing or feeding synthetic input streams)
            static auto parse(std::string_view bytes) -> Event;

        private:

            int m_signal_fd = -1;
            sigset_t m_old_mask{};
            std::string m_pending_input;

            auto on_resize() -> Event;
            auto parse_stdin() -> Event;

    };
}