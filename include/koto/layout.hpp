#pragma once

#include <vector>

namespace koto
{
    struct Area
    {
        int row, col;
        int width ,height;

        Area(int r, int c, int w, int h)
        : row(r),col(c),width(w),height(h)
        {}

        auto right() const -> int { return col + width ;}
        auto bottom() const -> int { return row + height ;}

        auto inset(int top, int left, int bottom, int right) const -> Area
        {
            int n_row = row + top;
            int n_col = col + left;
            int n_width = width > left + right ? width - left - right : 0;
            int n_height = height > top + bottom ? height - top - bottom : 0;

            n_width = (n_width < 0) ? 0 : n_width;
            n_height = (n_height < 0) ? 0 : n_height;

            return {n_row,n_col,n_width,n_height};
        }

        auto is_valid() const -> bool
        {
            return width > 0 && height > 0;
        }

        auto set_row(int r) const -> Area
        {
            Area copy = *this;
            copy.row = r;
            return copy;
        }
        auto set_col(int c) const -> Area
        {
            Area copy = *this;
            copy.col = c;
            return copy;
        }
        auto set_height(int h) const -> Area
        {
            Area copy = *this;
            copy.height = h;
            return copy;
        }
        auto set_width(int w) const -> Area
        {
            Area copy = *this;
            copy.width = w;
            return copy;
        }
    };

    struct Constraint
    {
        enum class Type {Fixed, Fill, Percent};
        Type type;
        int value = 0;

        static auto fixed(int n) -> Constraint { return {Type::Fixed, ((n < 0)?0:n)};}
        static auto fill() -> Constraint { return {Type::Fill, 0};}
        static auto percent(int n) -> Constraint { return {Type::Percent,((n < 0)?0:n) };}

    };

    namespace Layout
    {

        
        auto vertical(Area area, const std::vector<Constraint>& constraints) -> std::vector<Area>;
        auto horizontal(Area area, const std::vector<Constraint>& constraints) -> std::vector<Area>;
        auto center(Area area, int width, int height) -> Area;
        

    }
}