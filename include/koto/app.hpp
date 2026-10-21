#pragma once

#include "koto/event.hpp"
#include "koto/focus.hpp"
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
        App& on_mouse(std::function<void(const MouseEvent&)> handler);
        App& on_tick(std::function<void()> handler);
        App& on_render(std::function<void(Frame&)> handler);

        // Core focus integration
        // Automatically:
        // 1. Enables SGR mouse tracking on the terminal
        // 2. Syncs spatial geometry (frame.bounds_of) after every render pass
        // 3. Forwards unconsumed keys to fm.handle_key(key) (Tab, Arrows, typing)
        // 4. Routes left mouse clicks to fm.handle_mouse_click(row, col, frame)
        App& use_focus(FocusManager& fm);

        void quit();
        void run();
        void run(std::function<void(Frame&)> render_function);

        // Rendering & event dispatching (public for custom pipelines & headless testing)
        void draw_frame(bool flush = true);
        void dispatch_event(const Event& ev);
        Frame& frame() noexcept { return m_frame; }
        const Frame& frame() const noexcept { return m_frame; }

        private:
            Terminal m_term;
            Frame m_frame;
            EventLoop m_loop;

            bool m_running = true;
            bool m_needs_draw = true;

            FocusManager* m_focus_manager = nullptr;

            std::function<void(const KeyEvent&)> m_key_handler = nullptr;
            std::function<void(const std::string&)> m_paste_handler = nullptr;
            std::function<void(const MouseEvent&)> m_mouse_handler = nullptr;
            std::function<void()> m_tick_handler = nullptr;
            std::function<void(Frame&)> m_render_handler = nullptr;

    };
}