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

    void App::run(std::function<void (Frame &)> render_function)
    {
        m_render_handler = std::move(render_function);
        run();
    }

    void App::quit()
    {
        m_running = false;
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
                m_frame.clear_back_buffer();
                if (m_render_handler)
                {
                    m_render_handler(m_frame);
                }
                m_frame.render();
                m_needs_draw = false;
            }

            Event ev = m_loop.poll(16);
            if (auto* resize = ev.as_resize())
            {
                m_frame.handle_resize(static_cast<int>(resize->rows),static_cast<int>(resize->cols));
                m_needs_draw = true;
                continue;
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
                continue;
            }

            if (auto* key = ev.as_key())
            {
                if(key->ctrl && (key->ch == 'Q' || key->ch == 'q'))
                {
                    m_running = false;
                    break;
                }
                if (m_key_handler)
                {
                    m_key_handler(*key);
                    m_needs_draw = true;
                }
            }
        }
    }
}