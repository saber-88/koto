#pragma once
#include <cstdint>
#include <string_view>
#include "koto/color.hpp"
#include "koto/utf8.hpp"

namespace koto {

    enum class TextAttr : uint8_t
    {
        None        = 0,
        Bold        = 1 << 0,
        Dim         = 1 << 1,
        Italic      = 1 << 2,
        Underline   = 1 << 3,
        Reverse     = 1 << 4,
        Strike      = 1 << 5
    };

    inline constexpr uint8_t operator|(TextAttr a, TextAttr b) noexcept
    {
        return static_cast<uint8_t>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }
    inline constexpr uint8_t operator|(uint8_t a, TextAttr b) noexcept
    {
        return static_cast<uint8_t>(a | static_cast<uint8_t>(b));
    }

    struct Cell {
        Grapheme grapheme{};
        char32_t ch = U' ';
        uint32_t fg = COLOR_DEFAULT;
        uint32_t bg = COLOR_DEFAULT;
        int width = 1;
        bool continuation = false;
        uint8_t attrs = 0;

        constexpr Cell() noexcept = default;

        constexpr Cell(char32_t cp, uint32_t f = COLOR_DEFAULT, uint32_t b = COLOR_DEFAULT,
                       int w = 0, bool cont = false, uint8_t att = 0) noexcept
            : grapheme(cp, w > 0 ? w : (cp == 0 ? 0 : 1)), ch(cp), fg(f), bg(b),
              width(w > 0 ? w : (cp == 0 ? 0 : 1)), continuation(cont), attrs(att)
        {}

        constexpr Cell(Grapheme g, uint32_t f = COLOR_DEFAULT, uint32_t b = COLOR_DEFAULT,
                       bool cont = false, uint8_t att = 0) noexcept
            : grapheme(g), ch(g.base_cp), fg(f), bg(b), width(g.width), continuation(cont), attrs(att)
        {}

        constexpr Cell(Grapheme g, uint32_t f, uint32_t b, int w,
                       bool cont = false, uint8_t att = 0) noexcept
            : grapheme(g), ch(g.base_cp), fg(f), bg(b), width(w), continuation(cont), attrs(att)
        {
            grapheme.width = static_cast<uint8_t>(w > 0 ? w : 1);
        }

        [[nodiscard]] constexpr std::string_view view() const noexcept
        {
            return grapheme.view();
        }

        static constexpr Cell make_continuation() noexcept
        {
            Cell c{};
            c.continuation = true;
            return c;
        }

        bool operator==(const Cell&) const = default;
    };

}