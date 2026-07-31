#include "koto/color.hpp"
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

        char buf[7];
        color.copy(buf,color.size());

        return static_cast<uint32_t>(std::strtoul(buf,nullptr,16));
    }
}