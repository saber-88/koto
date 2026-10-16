#include "koto/color.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>

namespace koto
{
    auto parse_color(std::string_view color) -> uint32_t
    {   
        if (color.empty())
        {
            return COLOR_DEFAULT;
        }

        if (color[0] == '#')
        {
            color.remove_prefix(1);
        }

        char buf[8] = {};
        size_t n = std::min(sizeof(buf) - 1,color.size());
        color.copy(buf,n);
        buf[n] = '\0';
        return static_cast<uint32_t>(std::strtoul(buf,nullptr,16));
    }
}