#include "koto/event.hpp"
#include <bits/types/sigset_t.h>
#include <csignal>
#include <cstdint>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <unordered_map>
#include <string>
#include <sys/signalfd.h>

namespace koto
{
    static const std::unordered_map<std::string, Key> escape_map = {
        // arrow keys
        {"\033[A", Key::Up},
        {"\033[B", Key::Down},
        {"\033[C", Key::Right},
        {"\033[D", Key::Left},

        // shift + arrow
        {"\033[1;2A", Key::ShiftUp},
        {"\033[1;2B", Key::ShiftDown},
        {"\033[1;2C", Key::ShiftRight},
        {"\033[1;2D", Key::ShiftLeft},

        // ctrl + arrow
        {"\033[1;5A", Key::CtrlUp},
        {"\033[1;5B", Key::CtrlDown},
        {"\033[1;5C", Key::CtrlRight},
        {"\033[1;5D", Key::CtrlLeft},

        // alt + arrow
        {"\033[1;3A", Key::AltUp},
        {"\033[1;3B", Key::AltDown},
        {"\033[1;3C", Key::AltRight},
        {"\033[1;3D", Key::AltLeft},

        // navigation
        {"\033[H",  Key::Home},
        {"\033[F",  Key::End},
        {"\033[1~", Key::Home},  // alternate
        {"\033[4~", Key::End},   // alternate
        {"\033[5~", Key::PageUp},
        {"\033[6~", Key::PageDown},

        // editing
        {"\033[2~", Key::Insert},
        {"\033[3~", Key::Delete},

        // function keys F1-F4 (alternate sequences)
        {"\033OP",  Key::F1},
        {"\033OQ",  Key::F2},
        {"\033OR",  Key::F3},
        {"\033OS",  Key::F4},

        // function keys F1-F4 (vt sequence)
        {"\033[11~", Key::F1},
        {"\033[12~", Key::F2},
        {"\033[13~", Key::F3},
        {"\033[14~", Key::F4},

        // function keys F5-F12
        {"\033[15~", Key::F5},
        {"\033[17~", Key::F6},
        {"\033[18~", Key::F7},
        {"\033[19~", Key::F8},
        {"\033[20~", Key::F9},
        {"\033[21~", Key::F10},
        {"\033[23~", Key::F11},
        {"\033[24~", Key::F12},
    };

    auto EventLoop::poll(std::int32_t timeout_ms) -> Event
    {
        // return m_queue.pop(timeout_ms);
        struct pollfd pfds[2] =
        {
            {STDIN_FILENO, POLLIN,0},
            {m_signal_fd,POLLIN,0},
        };

        int ret = ::poll(pfds, 2, timeout_ms);

        if (ret < 0 || ret == 0)
        {
            return TickEvent{};
        }

        if (pfds[1].revents & POLLIN)
        {
            // resize
            struct signalfd_siginfo info;
            read(m_signal_fd, &info, sizeof(info));
            return on_resize();
        }

        if (pfds[0].revents & POLLIN)
        {
            // key
            return parse_stdin();
        }

        return TickEvent{};

    }

    auto EventLoop::parse_stdin() -> Event
    {
        char buf[8] = {};
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));

        if (n < 0)
        {
            return TickEvent{};
        }
        if (n >= 4 && buf[0] == '\033' && buf[1] == '[' && buf[2] == '1' && buf[3] == ';')
        {
            // shift / ctrl + arrow
            return parse_escape(buf, n);

        }
        if (n >= 3 && buf[0] == '\033' && buf[1] == '[')
        {
            // arrow key
            return parse_escape(buf,n);
        }
        if (n == 2 && buf[0] == '\033')
        {
            // alt key
            return KeyEvent{(uint32_t)buf[1],false,false,true};
        }
        if (n == 1 && buf[0] == 9)
        {
            // tab
            return KeyEvent{(uint32_t)Key::Tab};
        }
        if (n == 1 && (buf[0] == '\r' || buf[0] == '\n'))
        {
            //enter
            return KeyEvent{(uint32_t)Key::Enter};
        }
        if (n == 1 && buf[0] > 0 && buf[0] < 27)
        {
            // ctrl key
            return KeyEvent{(uint32_t)(buf[0] + 64), true};
        }
        if (n == 1 && buf[0] == '\033')
        {
            //escape key
            return KeyEvent{(uint32_t)Key::Escape};
        }
        if (n == 1 && buf[0] == 127)
        {
            // backspace
            return KeyEvent{(uint32_t)Key::Backspace};
        }
        if (n == 1 && buf[0] >= 32)
        {
            // regular char
            return KeyEvent{(uint32_t)buf[0]};
        }
        
        return TickEvent{};
    }

    auto EventLoop::parse_escape(const char *buf, ssize_t n) -> Event
    {
        std::string seq(buf,n);
        auto it = escape_map.find(seq);
        if(it != escape_map.end())
        {
            return KeyEvent{(uint32_t)it->second};
        }

        return TickEvent{};
    }

    
    auto EventLoop::on_resize() -> Event
    {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ,&w);
        uint32_t r = w.ws_row > 0 ? w.ws_row : 1;
        uint32_t c = w.ws_col > 0 ? w.ws_col : 1;
        return ResizeEvent{r,c};
    }

    EventLoop::EventLoop()
    {   
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGWINCH);
        sigprocmask(SIG_BLOCK, &mask, nullptr);
        m_signal_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);

    }

    EventLoop::~EventLoop()
    {
        if (m_signal_fd > 0)
        {
            close(m_signal_fd);
        }
    }

    
}