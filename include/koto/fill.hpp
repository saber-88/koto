#pragma once

#include "koto/cell.hpp"
#include "koto/color.hpp"
#include "koto/frame.hpp"
#include "koto/id.hpp"
#include "koto/layout.hpp"
#include "koto/widget.hpp"
#include <cstdint>
#include <string_view>

namespace koto
{
    class Fill : public Widget
    {
    public:
        Fill() = default;
        explicit Fill(Color bg);

        Fill& bg(Color color);
        Fill& fg(Color color);
        Fill& char_code(char32_t ch);
        Fill& id(Id id);

        void draw(Frame& frame, Area area) override;

    private:
        char32_t m_ch = U' ';
        uint32_t m_fg = COLOR_DEFAULT;
        uint32_t m_bg = COLOR_DEFAULT;
        Id m_tag_id = Id::none();
    };

    class Clear : public Widget
    {
    public:
        Clear() = default;
        Clear& id(Id id);

        void draw(Frame& frame, Area area) override;

    private:
        Id m_tag_id = Id::none();
    };
}
