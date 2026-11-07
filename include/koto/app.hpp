#pragma once

#include "koto/event.hpp"
#include "koto/focus.hpp"
#include "koto/frame.hpp"
#include "koto/terminal.hpp"
#include <atomic>
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
        App& on_tick(std::function<void(double)> handler);
        App& on_render(std::function<void(Frame&)> handler);

        // Core focus integration
        // Automatically:
        // 1. Enables SGR mouse tracking on the terminal
        // 2. Syncs spatial geometry (frame.bounds_of) after every render pass
        // 3. Forwards unconsumed keys to fm.handle_key(key) (Tab, Arrows, typing)
        // 4. Routes left mouse clicks to fm.handle_mouse_click(row, col, frame)
        App& use_focus(FocusManager& fm);

        void quit();
        void request_redraw() noexcept;
        auto run() -> std::expected<void, TerminalErrorInfo>;
        auto run(std::function<void(Frame&)> render_function) -> std::expected<void, TerminalErrorInfo>;

        // Screen geometry & hit-testing (abstracted away from Frame)
        Area area() const noexcept;
        Id hit_at(int row, int col) const;
        Id hit_at(const MouseEvent& mouse) const;

        // Rendering & event dispatching (public for custom pipelines & headless testing)
        void draw_frame(bool flush = true);
        void dispatch_event(const Event& ev);
        Frame& frame() noexcept { return m_frame; }
        const Frame& frame() const noexcept { return m_frame; }

        private:
            Terminal m_term;
            Frame m_frame;
            EventLoop m_loop;

            std::atomic<bool> m_running{true};
            std::atomic<bool> m_needs_draw{true};

            FocusManager* m_focus_manager = nullptr;

            std::function<void(const KeyEvent&)> m_key_handler = nullptr;
            std::function<void(const std::string&)> m_paste_handler = nullptr;
            std::function<void(const MouseEvent&)> m_mouse_handler = nullptr;
            std::function<void()> m_tick_handler = nullptr;
            std::function<void(double)> m_tick_dt_handler = nullptr;
            std::function<void(Frame&)> m_render_handler = nullptr;

    };
}