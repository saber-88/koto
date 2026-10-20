#pragma once

#include <cstdint>
#include <string_view>

namespace koto
{
    struct Utf8Buffer
    {
        char data[5] = {};
        uint8_t size = 0;
        constexpr std::string_view view() const noexcept { return std::string_view(data, size); }
        constexpr const char* c_str() const noexcept { return data; }
    };

    // Modern C++23 bounds-safe stream decoder (advances in by consumed codepoint bytes)
    char32_t next_codepoint(std::string_view& in);

    // Modern C++23 value-returning encoder
    Utf8Buffer encode_utf8(char32_t code);

    // Backward-compatible C-style overloads
    char32_t next_codepoint(const char*& ptr, const char* end);
    int encode_utf8(char32_t code , char*);

    int codepoint_width(char32_t cp);
    int text_width(std::string_view text);
}