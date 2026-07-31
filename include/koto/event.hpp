#pragma once

#include <cstdint>
#include <variant>
#include <unistd.h>

namespace koto
{
    enum class Key : uint32_t {

        // navigation
        Up = 256, Down, Left, Right,
        Home, End, PageUp, PageDown,

        // editing
        Enter, Backspace, Delete, Insert, Tab,

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
        uint32_t ch;
        bool ctrl = false;
        bool shift = false;
        bool alt = false;

        bool is(Key k) const { return ch == (uint32_t)k ;}
    };

    struct ResizeEvent
    {
        uint32_t rows;
        uint32_t cols;
    };

    struct TickEvent
    {};

    using Event = std::variant<KeyEvent, ResizeEvent, TickEvent>;


    class EventLoop
    {
        private:

            int m_signal_fd = -1;
            auto on_resize() -> Event;
            auto parse_stdin() -> Event;
            auto parse_escape(const char *buf, ssize_t n) -> Event;

        public: 

            EventLoop();
            ~EventLoop();
            auto poll(std::int32_t ms = 16) -> Event;


    };
}