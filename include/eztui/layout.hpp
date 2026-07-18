#pragma once

#include <cstdint>
#include <vector>
namespace eztui
{

    
    struct Rect
    {
        uint32_t row, col;
        uint32_t width ,height;

        uint32_t right() const { return col + width ;}
        uint32_t bottom() const { return row + height ;}
    };

    struct Constraint
    {
        enum class Type {Fixed, Fill, Percent};
        Type type;
        uint32_t value = 0;

        static Constraint fixed(uint32_t n) { return {Type::Fixed, n};}
        static Constraint fill() { return {Type::Fill, 0};}
        static Constraint percent(uint32_t n) { return {Type::Percent, n};}

    };

    namespace Layout
    {

        std::vector<Rect> vertical(Rect area, std::vector<Constraint> constraints);
        std::vector<Rect> horizontal(Rect area, std::vector<Constraint> constraints);

    }
}