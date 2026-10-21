#include "koto/app.hpp"
#include "koto/event.hpp"
#include "koto/frame.hpp"
#include "koto/utf8.hpp"

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
            if (m_mouse_handler)
            {
                m_mouse_handler(*mouse);
            }
            if (m_focus_manager && mouse->action == MouseAction::Press && mouse->button == MouseButton::Left)
            {
                if (m_focus_manager->handle_mouse_click(mouse->row, mouse->col, m_frame))
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

        while (m_running)
        {
            if (m_tick_handler)
            {
                m_tick_handler();
                m_needs_draw = true;
            }

            if (m_needs_draw)
            {
                this->draw_frame();
            }

            Event ev = m_loop.poll(16);
            if (!ev.is_tick())
            {
                this->dispatch_event(ev);
            }
        }
    }
}