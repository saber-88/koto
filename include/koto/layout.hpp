#pragma once

#include <cstdint>
#include <vector>
namespace koto
{

    
    struct Area
    {
        uint32_t row, col;
        uint32_t width ,height;

        Area(uint32_t r, uint32_t c, uint32_t w, uint32_t h)
        : row(r),col(c),width(w),height(h)
        {}

        auto right() -> uint32_t  const { return col + width ;}
        auto bottom() -> uint32_t const { return row + height ;}

        auto inset(uint32_t top, uint32_t left, uint32_t bottom, uint32_t right) const -> Area
        {
            uint32_t n_row = row + top;
            uint32_t n_col = col + left;
            uint32_t n_width = width > left + right ? width - left - right : 0;
            uint32_t n_height = height > top + bottom ? height - top - bottom : 0;

            return {n_row,n_col,n_width,n_height};
        }

        auto is_valid() const -> bool
        {
            return width > 0 && height > 0;
        }
    };

    struct Constraint
    {
        enum class Type {Fixed, Fill, Percent};
        Type type;
        uint32_t value = 0;

        static auto fixed(uint32_t n) -> Constraint { return {Type::Fixed, n};}
        static auto fill() -> Constraint { return {Type::Fill, 0};}
        static auto percent(uint32_t n) -> Constraint { return {Type::Percent, n};}

    };

    namespace Layout
    {

        
        auto vertical(Area area, std::vector<Constraint> constraints) -> std::vector<Area>;
        auto horizontal(Area area, std::vector<Constraint> constraints) -> std::vector<Area>;
        auto center(Area area, uint32_t width, uint32_t height) -> Area;
        

    }
}