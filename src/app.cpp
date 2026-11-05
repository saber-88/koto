#include "koto/app.hpp"
#include "koto/event.hpp"
#include "koto/frame.hpp"
#include "koto/utf8.hpp"
#include <algorithm>
#include <chrono>

namespace koto
{
    App::App()
        : m_term(),
          m_frame(m_term),
          m_loop()
    {}

    App& App::on_key(std::function<void(const KeyEvent&)> handler)
    {
        m_key_handler = std::move(handler);
        return *this;
    }
    App& App::on_paste(std::function<void(const std::string&)> handler)
    {
        m_paste_handler = std::move(handler);
        return *this;
    }
    App& App::on_mouse(std::function<void(const MouseEvent&)> handler)
    {
        m_mouse_handler = std::move(handler);
        m_term.enable_mouse_capture();
        return *this;
    }
    App& App::on_tick(std::function<void()> handler)
    {
        m_tick_handler = std::move(handler);
        return *this;
    }
    App& App::on_render(std::function<void(Frame&)> handler)
    {
        m_render_handler = std::move(handler);
        return *this;
    }

    App& App::use_focus(FocusManager& fm)
    {
        m_focus_manager = &fm;
        m_term.enable_mouse_capture();
        return *this;
    }

    void App::run(std::function<void (Frame &)> render_function)
    {
        m_render_handler = std::move(render_function);
        run();
    }

    Area App::area() const noexcept
    {
        return m_frame.area();
    }

    Id App::hit_at(int row, int col) const
    {
        return m_frame.hit_at(row, col);
    }

    Id App::hit_at(const MouseEvent& mouse) const
    {
        return m_frame.hit_at(mouse.row, mouse.col);
    }

    void App::quit()
    {
        m_running = false;
    }

    void App::dispatch_event(const Event& ev)
    {
        if (auto* resize = ev.as_resize())
        {
            m_frame.handle_resize(static_cast<int>(resize->rows), static_cast<int>(resize->cols));
            m_needs_draw = true;
            return;
        }

        if (auto* paste = ev.as_paste())
        {
            if (m_paste_handler)
            {
                m_paste_handler(paste->text);
                m_needs_draw = true;
            }
            else if (m_key_handler)
            {
                std::string_view remaining = paste->text;
                while (!remaining.empty())
                {
                    char32_t cp = next_codepoint(remaining);
                    if (cp != 0)
                    {
                        KeyEvent ke{};
                        if (cp == '\n')
                        {
                            ke.ch = static_cast<uint32_t>(Key::Enter);
                        }
                        else if (cp == '\t')
                        {
                            ke.ch = static_cast<uint32_t>(Key::Tab);
                        }
                        else
                        {
                            ke.ch = static_cast<uint32_t>(cp);
                            if (cp >= 'A' && cp <= 'Z') ke.shift = true;
                        }
                        m_key_handler(ke);
                    }
                }
                m_needs_draw = true;
            }
            return;
        }

        if (auto* mouse = ev.as_mouse())
        {
            MouseEvent m = *mouse;
            m.target = m_frame.hit_at(m.row, m.col);

            if (m_mouse_handler)
            {
                m_mouse_handler(m);
            }
            if (m_focus_manager)
            {
                if (m_focus_manager->handle_mouse(m, m_frame))
                {
                    m_needs_draw = true;
                }
            }
            return;
        }

        if (auto* key = ev.as_key())
        {
            if (key->ctrl && (key->ch == 'Q' || key->ch == 'q'))
            {
                m_running = false;
                return;
            }

            bool consumed = false;
            if (m_focus_manager)
            {
                consumed = m_focus_manager->handle_key(*key);
                if (consumed)
                {
                    m_needs_draw = true;
                }
            }

            if (!consumed && m_key_handler)
            {
                m_key_handler(*key);
                m_needs_draw = true;
            }
        }
    }

    void App::draw_frame(bool flush)
    {
        m_frame.clear_back_buffer();
        if (m_render_handler)
        {
            m_render_handler(m_frame);
        }
        if (m_focus_manager)
        {
            m_focus_manager->sync_geometry(m_frame);
        }
        m_frame.render(flush);
        m_needs_draw = false;
    }

    void App::run()
    {
        m_term.hide_cursor();
        m_running = true;
        m_needs_draw = true;

        using clock = std::chrono::steady_clock;
        auto next_frame = clock::now();

        while (m_running)
        {
            // 1. Calculate wait timeout:
            // - If events are already pending in memory or we need a redraw, don't wait (0ms).
            // - If an animation tick handler is active, wait for the remaining time in this 16ms frame (~60 FPS).
            // - If static (no animation tick handler), sleep in kernel poll indefinitely (-1ms) until input arrives.
            int32_t wait_ms = -1;
            if (m_loop.has_pending() || m_needs_draw)
            {
                wait_ms = 0;
            }
            else if (m_tick_handler)
            {
                auto now = clock::now();
                if (now < next_frame)
                {
                    auto rem = std::chrono::duration_cast<std::chrono::milliseconds>(next_frame - now).count();
                    wait_ms = static_cast<int32_t>(std::clamp(rem, 0L, 16L));
                }
                else
                {
                    wait_ms = 0;
                }
            }

            Event ev = m_loop.poll(wait_ms);
            if (!ev.is_tick())
            {
                this->dispatch_event(ev);
            }

            // 2. Event Draining / Batching:
            // Consume ALL pending input events that accumulated in the buffer or stdin
            // BEFORE executing the tick and render passes. This prevents keyboard auto-repeat
            // (holding arrow keys) from building up an event backlog that causes lag propagation.
            while (m_running)
            {
                Event pending_ev = m_loop.poll(0);
                if (pending_ev.is_tick())
                {
                    break;
                }
                this->dispatch_event(pending_ev);
            }

            if (!m_running) break;

            // 3. Periodic animation tick (triggered when frame time elapses)
            if (m_tick_handler)
            {
                auto now = clock::now();
                if (now >= next_frame)
                {
                    m_tick_handler();
                    m_needs_draw = true;
                    next_frame = now + std::chrono::milliseconds(16);
                }
            }

            // 4. Render pass (rendered AT MOST ONCE per frame for all drained inputs)
            if (m_needs_draw)
            {
                this->draw_frame();
            }
        }
    }
}