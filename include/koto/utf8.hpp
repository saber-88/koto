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

    struct Grapheme
    {
        char data[36] = {' ', '\0'};
        uint8_t size = 1;
        uint8_t width = 1;
        char32_t base_cp = U' ';

        constexpr Grapheme() noexcept = default;

        constexpr Grapheme(char32_t cp, int w = 1) noexcept
            : width(static_cast<uint8_t>(w > 0 ? w : 1)), base_cp(cp)
        {
            if (cp == 0)
            {
                data[0] = '\0';
                size = 0;
                width = 0;
                base_cp = 0;
            }
            else if (cp < 0x80)
            {
                data[0] = static_cast<char>(cp);
                data[1] = '\0';
                size = 1;
            }
            else if (cp < 0x800)
            {
                data[0] = static_cast<char>(0xC0 | (static_cast<uint32_t>(cp) >> 6));
                data[1] = static_cast<char>(0x80 | (static_cast<uint32_t>(cp) & 0x3Fu));
                data[2] = '\0';
                size = 2;
            }
            else if (cp < 0x10000)
            {
                data[0] = static_cast<char>(0xE0 | (static_cast<uint32_t>(cp) >> 12));
                data[1] = static_cast<char>(0x80 | ((static_cast<uint32_t>(cp) >> 6) & 0x3Fu));
                data[2] = static_cast<char>(0x80 | (static_cast<uint32_t>(cp) & 0x3Fu));
                data[3] = '\0';
                size = 3;
            }
            else
            {
                data[0] = static_cast<char>(0xF0 | (static_cast<uint32_t>(cp) >> 18));
                data[1] = static_cast<char>(0x80 | ((static_cast<uint32_t>(cp) >> 12) & 0x3Fu));
                data[2] = static_cast<char>(0x80 | ((static_cast<uint32_t>(cp) >> 6) & 0x3Fu));
                data[3] = static_cast<char>(0x80 | (static_cast<uint32_t>(cp) & 0x3Fu));
                data[4] = '\0';
                size = 4;
            }
        }

        constexpr Grapheme(std::string_view sv, char32_t cp = 0, int w = 1) noexcept
            : width(static_cast<uint8_t>(w > 0 ? w : 1)), base_cp(cp)
        {
            if (sv.empty())
            {
                data[0] = '\0';
                size = 0;
                width = 0;
                base_cp = 0;
                return;
            }

            // Copy full codepoints only - never slice mid-codepoint
            constexpr size_t max_bytes = sizeof(data) - 1;
            size_t valid_len = 0;
            size_t i = 0;
            while (i < sv.size())
            {
                unsigned char c = static_cast<unsigned char>(sv[i]);
                size_t cp_len = 1;
                if ((c & 0x80) == 0) cp_len = 1;
                else if ((c & 0xE0) == 0xC0) cp_len = 2;
                else if ((c & 0xF0) == 0xE0) cp_len = 3;
                else if ((c & 0xF8) == 0xF0) cp_len = 4;
                else cp_len = 1;

                if (i + cp_len > sv.size()) break;
                if (valid_len + cp_len > max_bytes) break;

                valid_len += cp_len;
                i += cp_len;
            }

            for (size_t k = 0; k < valid_len; ++k)
            {
                data[k] = sv[k];
            }
            data[valid_len] = '\0';
            for (size_t k = valid_len + 1; k < sizeof(data); ++k)
            {
                data[k] = '\0';
            }
            size = static_cast<uint8_t>(valid_len);
        }

        [[nodiscard]] constexpr std::string_view view() const noexcept { return std::string_view(data, size); }
        [[nodiscard]] constexpr const char* c_str() const noexcept { return data; }
        [[nodiscard]] constexpr bool empty() const noexcept { return size == 0; }

        bool operator==(const Grapheme&) const = default;
    };

    // Advances in by the next extended grapheme cluster and returns its metadata
    Grapheme next_grapheme(std::string_view& in);

    // Returns the byte offset of the grapheme cluster start immediately before 'cursor' (or 0)
    size_t prev_grapheme_offset(std::string_view text, size_t cursor) noexcept;

    // Returns the byte offset of the grapheme cluster end immediately after 'cursor' (or text.size())
    size_t next_grapheme_offset(std::string_view text, size_t cursor) noexcept;

    // Converts a visual display column to the corresponding byte offset in text
    size_t visual_col_to_byte_offset(std::string_view text, int target_visual_col) noexcept;

    bool is_unicode_wide(char32_t cp) noexcept;
    bool is_unicode_zero_width(char32_t cp) noexcept;
    int codepoint_width(char32_t cp);
    int text_width(std::string_view text);
}