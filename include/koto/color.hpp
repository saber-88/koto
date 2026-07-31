#pragma once

#include <cstdint>
#include <string_view>

namespace koto
{

    inline constexpr uint32_t COLOR_DEFAULT = 0xFF000000;
    auto parse_color(std::string_view color) -> uint32_t;

}