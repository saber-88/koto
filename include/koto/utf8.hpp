#pragma once

#include <cstdint>
namespace koto
{

    inline auto next_codepoint(const char*& ptr, const char* end) -> char32_t {
        uint8_t byte = (uint8_t)*ptr;

        if (byte < 0x80) {
            // single byte — ASCII
            ptr += 1;
            return byte;
        }
        else if (byte < 0xE0) {
            // 2 bytes
            char32_t cp = (byte & 0x1F) << 6;
            cp |= ((uint8_t)ptr[1] & 0x3F);
            ptr += 2;
            return cp;
        }
        else if (byte < 0xF0) {
            // 3 bytes
            char32_t cp = (byte & 0x0F) << 12;
            cp |= ((uint8_t)ptr[1] & 0x3F) << 6;
            cp |= ((uint8_t)ptr[2] & 0x3F);
            ptr += 3;
            return cp;
        }
        else {
            // 4 bytes
            char32_t cp = (byte & 0x07) << 18;
            cp |= ((uint8_t)ptr[1] & 0x3F) << 12;
            cp |= ((uint8_t)ptr[2] & 0x3F) << 6;
            cp |= ((uint8_t)ptr[3] & 0x3F);
            ptr += 4;
            return cp;
        }
    }
}