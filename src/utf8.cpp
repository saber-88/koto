#include "koto/utf8.hpp"
#include <cstdint>
#include <string_view>
#include <wchar.h>


namespace koto
{
    char32_t next_codepoint(std::string_view& in)
    {
        if (in.empty()) return 0;

        uint8_t byte = static_cast<uint8_t>(in[0]);

        // 1-byte ASCII (0x00 - 0x7F)
        if (byte < 0x80)
        {
            in.remove_prefix(1);
            return byte;
        }

        // Stray continuation byte (0x80 - 0xBF) or overlong 2-byte lead (0xC0 - 0xC1)
        if (byte < 0xC2 || byte >= 0xF5)
        {
            in.remove_prefix(1);
            return U'\uFFFD';
        }

        // 2-byte sequence (0xC2 - 0xDF)
        if (byte < 0xE0)
        {
            if (in.size() < 2 || (static_cast<uint8_t>(in[1]) & 0xC0) != 0x80)
            {
                in.remove_prefix(1);
                return U'\uFFFD';
            }
            char32_t cp = static_cast<char32_t>((static_cast<uint32_t>(byte) & 0x1Fu) << 6);
            cp |= static_cast<char32_t>(static_cast<uint32_t>(static_cast<uint8_t>(in[1])) & 0x3Fu);
            in.remove_prefix(2);
            return cp;
        }

        // 3-byte sequence (0xE0 - 0xEF)
        if (byte < 0xF0)
        {
            if (in.size() < 2)
            {
                in.remove_prefix(1);
                return U'\uFFFD';
            }
            uint8_t b1 = static_cast<uint8_t>(in[1]);
            // Disallow overlong (< 0x800) and UTF-16 surrogates (0xD800 - 0xDFFF)
            if (byte == 0xE0 && (b1 < 0xA0 || b1 > 0xBF))
            {
                in.remove_prefix(1);
                return U'\uFFFD';
            }
            if (byte == 0xED && (b1 < 0x80 || b1 > 0x9F))
            {
                in.remove_prefix(1);
                return U'\uFFFD';
            }
            if ((b1 & 0xC0) != 0x80)
            {
                in.remove_prefix(1);
                return U'\uFFFD';
            }
            if (in.size() < 3 || (static_cast<uint8_t>(in[2]) & 0xC0) != 0x80)
            {
                in.remove_prefix(2);
                return U'\uFFFD';
            }

            char32_t cp = static_cast<char32_t>((static_cast<uint32_t>(byte) & 0x0Fu) << 12);
            cp |= static_cast<char32_t>((static_cast<uint32_t>(b1) & 0x3Fu) << 6);
            cp |= static_cast<char32_t>(static_cast<uint32_t>(static_cast<uint8_t>(in[2])) & 0x3Fu);
            in.remove_prefix(3);
            return cp;
        }

        // 4-byte sequence (0xF0 - 0xF4)
        if (byte <= 0xF4)
        {
            if (in.size() < 2)
            {
                in.remove_prefix(1);
                return U'\uFFFD';
            }
            uint8_t b1 = static_cast<uint8_t>(in[1]);
            // Disallow overlong (< 0x10000) and codepoints > 0x10FFFF
            if (byte == 0xF0 && (b1 < 0x90 || b1 > 0xBF))
            {
                in.remove_prefix(1);
                return U'\uFFFD';
            }
            if (byte == 0xF4 && (b1 < 0x80 || b1 > 0x8F))
            {
                in.remove_prefix(1);
                return U'\uFFFD';
            }
            if ((b1 & 0xC0) != 0x80)
            {
                in.remove_prefix(1);
                return U'\uFFFD';
            }
            if (in.size() < 3 || (static_cast<uint8_t>(in[2]) & 0xC0) != 0x80)
            {
                in.remove_prefix(2);
                return U'\uFFFD';
            }
            if (in.size() < 4 || (static_cast<uint8_t>(in[3]) & 0xC0) != 0x80)
            {
                in.remove_prefix(3);
                return U'\uFFFD';
            }

            char32_t cp = static_cast<char32_t>((static_cast<uint32_t>(byte) & 0x07u) << 18);
            cp |= static_cast<char32_t>((static_cast<uint32_t>(b1) & 0x3Fu) << 12);
            cp |= static_cast<char32_t>((static_cast<uint32_t>(static_cast<uint8_t>(in[2])) & 0x3Fu) << 6);
            cp |= static_cast<char32_t>(static_cast<uint32_t>(static_cast<uint8_t>(in[3])) & 0x3Fu);
            in.remove_prefix(4);
            return cp;
        }

        in.remove_prefix(1);
        return U'\uFFFD';
    }

    char32_t next_codepoint(const char*& ptr, const char* end)
    {
        if (ptr >= end) return 0;
        std::string_view sv(ptr, static_cast<size_t>(end - ptr));
        char32_t cp = next_codepoint(sv);
        ptr = sv.data();
        return cp;
    }

    Utf8Buffer encode_utf8(char32_t code)
    {
        Utf8Buffer buf{};
        int len = encode_utf8(code, buf.data);
        buf.size = len > 0 ? static_cast<uint8_t>(len) : static_cast<uint8_t>(0);
        return buf;
    }

    int encode_utf8(char32_t code , char* utf8)
    {
        int len = 0;
        if (code < 0x80)
        {
            utf8[0] = static_cast<char>(code);
            return 1;
        }
        else if (code < 0x800)
        {
            utf8[0] =static_cast<char>( 0xC0 | (code >> 6));
            utf8[1] = static_cast<char>(0x80 | (code & 0x3F));
            return 2;
        }
        else if (code < 0x10000)
        {
            utf8[0] = static_cast<char>(0xE0 | (code >> 12));
            utf8[1] = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            utf8[2] = static_cast<char>(0x80 | (code & 0x3F));
            return 3;
        }
        else
        {
            utf8[0] = static_cast<char>(0xF0 | (code >> 18));
            utf8[1] = static_cast<char>(0x80 | ((code >> 12) & 0x3F));
            utf8[2] = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            utf8[3] = static_cast<char>(0x80 | (code & 0x3F));

            return 4;
        }
        return len;
        
    }

    #include "unicode_data.inc"

    static bool in_interval_table(char32_t cp, const UnicodeInterval* table, size_t size) noexcept
    {
        int l = 0;
        int r = static_cast<int>(size) - 1;
        while (l <= r)
        {
            int m = l + (r - l) / 2;
            if (cp < table[m].first)
            {
                r = m - 1;
            }
            else if (cp > table[m].last)
            {
                l = m + 1;
            }
            else
            {
                return true;
            }
        }
        return false;
    }

    bool is_unicode_zero_width(char32_t cp) noexcept
    {
        if (cp == 0 || cp == 0x200B || cp == 0xFEFF) return true;
        if (cp < 0x0300) return false;
        return in_interval_table(cp, UNICODE_ZERO_RANGES, sizeof(UNICODE_ZERO_RANGES) / sizeof(UNICODE_ZERO_RANGES[0]));
    }

    bool is_unicode_wide(char32_t cp) noexcept
    {
        if (cp < 0x1100) return false;
        return in_interval_table(cp, UNICODE_WIDE_RANGES, sizeof(UNICODE_WIDE_RANGES) / sizeof(UNICODE_WIDE_RANGES[0]));
    }

    int codepoint_width(char32_t cp)
    {
        // 1. ASCII fast path: 0x20..0x7E are always width 1
        if (cp >= 0x20 && cp < 0x7F)
        {
            return 1;
        }

        // 2. Control characters (C0 & C1)
        if (cp < 0x20 || (cp >= 0x7F && cp < 0xA0))
        {
            return 0;
        }

        // 3. Zero-width combining marks & formatting characters
        if (is_unicode_zero_width(cp))
        {
            return 0;
        }

        // 4. Unicode Wide/Fullwidth (UAX #11) + UTS #51 default Emoji Presentation
        if (is_unicode_wide(cp))
        {
            return 2;
        }

        return 1;
    }

    Grapheme next_grapheme(std::string_view& in)
    {
        if (in.empty()) return {};

        const char* start = in.data();
        char32_t base_cp = next_codepoint(in);
        if (base_cp == 0) return {};

        // 1. ASCII fast-path (check for keycap sequence '0'..'9', '#', '*')
        if (base_cp < 0x80)
        {
            if ((base_cp >= '0' && base_cp <= '9') || base_cp == '#' || base_cp == '*')
            {
                std::string_view peek = in;
                if (!peek.empty())
                {
                    char32_t c1 = next_codepoint(peek);
                    if (c1 == 0xFE0F && !peek.empty())
                    {
                        char32_t c2 = next_codepoint(peek);
                        if (c2 == 0x20E3) // Keycap combination (e.g. 1️⃣)
                        {
                            in = peek;
                            return {std::string_view(start, static_cast<size_t>(in.data() - start)), base_cp, 2};
                        }
                    }
                }
            }
            int w = (base_cp < 0x20) ? 0 : 1;
            return {std::string_view(start, 1), base_cp, w};
        }

        // 2. Regional Indicator Pair (Flags: 0x1F1E6..0x1F1FF)
        if (base_cp >= 0x1F1E6 && base_cp <= 0x1F1FF)
        {
            std::string_view peek = in;
            if (!peek.empty())
            {
                char32_t second = next_codepoint(peek);
                if (second >= 0x1F1E6 && second <= 0x1F1FF)
                {
                    in = peek;
                    return {std::string_view(start, static_cast<size_t>(in.data() - start)), base_cp, 2};
                }
            }
        }

        int cluster_w = codepoint_width(base_cp);

        // 3. Consume trailing Variation Selectors, skin-tone modifiers, ZWJ sequences, and combining marks
        while (!in.empty())
        {
            std::string_view peek = in;
            char32_t next_cp = next_codepoint(peek);
            if (next_cp == 0) break;

            if (next_cp == 0xFE0F) // Variation Selector 16: force emoji presentation
            {
                in = peek;
                cluster_w = 2;
                continue;
            }
            if (next_cp == 0xFE0E) // Variation Selector 15: force text presentation
            {
                in = peek;
                cluster_w = 1;
                continue;
            }
            if (next_cp >= 0x1F3FB && next_cp <= 0x1F3FF) // Skin tone modifier
            {
                in = peek;
                cluster_w = 2;
                continue;
            }
            if (next_cp == 0x200D) // Zero-Width Joiner
            {
                in = peek;
                if (!in.empty())
                {
                    char32_t joined = next_codepoint(in);
                    (void)joined;
                    std::string_view trail_peek = in;
                    if (!trail_peek.empty())
                    {
                        char32_t trail = next_codepoint(trail_peek);
                        if (trail == 0xFE0F || (trail >= 0x1F3FB && trail <= 0x1F3FF))
                        {
                            in = trail_peek;
                        }
                    }
                }
                cluster_w = 2;
                continue;
            }
            if (is_unicode_zero_width(next_cp))
            {
                in = peek;
                continue;
            }
            break;
        }

        return {std::string_view(start, static_cast<size_t>(in.data() - start)), base_cp, cluster_w};
    }

    int text_width(std::string_view text)
    {
        int total = 0;
        std::string_view rem = text;
        while (!rem.empty())
        {
            Grapheme g = next_grapheme(rem);
            if (g.empty()) break;
            if (g.base_cp == U'\t')
            {
                int tab_size = 4;
                total = ((total / tab_size) + 1) * tab_size;
            }
            else
            {
                total += g.width;
            }
        }
        return total;
    }

    size_t prev_grapheme_offset(std::string_view text, size_t cursor) noexcept
    {
        if (cursor == 0 || text.empty()) return 0;
        cursor = std::min(cursor, text.size());

        size_t prev = 0;
        std::string_view rem = text;
        while (!rem.empty())
        {
            size_t current = static_cast<size_t>(rem.data() - text.data());
            if (current >= cursor) break;
            prev = current;
            Grapheme g = next_grapheme(rem);
            if (g.empty()) break;
        }
        return prev;
    }

    size_t next_grapheme_offset(std::string_view text, size_t cursor) noexcept
    {
        if (cursor >= text.size()) return text.size();

        std::string_view rem = text.substr(cursor);
        Grapheme g = next_grapheme(rem);
        if (g.empty()) return text.size();
        return cursor + static_cast<size_t>(g.size);
    }

    size_t visual_col_to_byte_offset(std::string_view text, int target_visual_col) noexcept
    {
        if (target_visual_col <= 0 || text.empty()) return 0;

        int current_visual = 0;
        std::string_view rem = text;
        while (!rem.empty())
        {
            size_t current_offset = static_cast<size_t>(rem.data() - text.data());
            Grapheme g = next_grapheme(rem);
            if (g.empty()) break;
            int w = (g.base_cp == U'\t') ? 4 : g.width;
            if (current_visual + w > target_visual_col)
            {
                return current_offset;
            }
            current_visual += w;
            if (current_visual == target_visual_col)
            {
                return static_cast<size_t>(rem.data() - text.data());
            }
        }
        return text.size();
    }

}