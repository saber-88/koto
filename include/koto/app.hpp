#pragma once

#include "koto/event.hpp"
#include "koto/frame.hpp"
#include "koto/terminal.hpp"
#include <functional>
namespace koto
{

    class App
    {
        public:
        App();
        ~App() = default;

        App(const App&) = delete;
        App& operator=(const App&) = delete;

        App& on_key(std::function<void(const KeyEvent&)> handler);
        App& on_paste(std::function<void(const std::string&)> handler);
        App& on_tick(std::function<void()> handler);
        App& on_render(std::function<void(Frame&)> handler);

        void quit();
        void run();
        void run(std::function<void(Frame&)> render_function);

        private:
            Terminal m_term;
            Frame m_frame;
            EventLoop m_loop;

            bool m_running = true;
            bool m_needs_draw = true;

            std::function<void(const KeyEvent&)> m_key_handler = nullptr;
            std::function<void(const std::string&)> m_paste_handler = nullptr;
            std::function<void()> m_tick_handler = nullptr;
            std::function<void(Frame&)> m_render_handler = nullptr;

    };
}