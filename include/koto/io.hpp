#pragma once

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <expected>
#include <string_view>
#include <sys/types.h>
#include <unistd.h>

namespace koto
{
    [[nodiscard]] inline std::expected<void, int> write_all(int fd, std::string_view data){
        const char*  ptr = data.data();
        size_t rem = data.size();
        while (rem > 0)
        {
            ssize_t written = ::write(fd, ptr, rem);
            if (written < 0)
            {
                if (errno == EINTR) continue;
                return std::unexpected<int>(errno);
            }
            ptr += written;
            rem -= static_cast<size_t>(written);
        }
        return {};
    }
}