#include "koto/utf8.hpp"
#include <clocale>
#include <cstdint>
#include <string_view>
#include <wchar.h>


namespace koto
{
    char32_t next_codepoint(std::string_view& in)
    {
        if (in.empty()) return 0;

        uint8_t byte = static_cast<uint8_t>(in[0]);

        if (byte < 0x80)
        {
            in.remove_prefix(1);
            return byte;
        }
        else if (byte < 0xE0)
        {
            if (in.size() < 2) { in = {}; return U'\uFFFD'; }
            char32_t cp = static_cast<char32_t>((byte & 0x1F) << 6);
            cp |= (static_cast<uint8_t>(in[1]) & 0x3F);
            in.remove_prefix(2);
            return cp;
        }
        else if (byte < 0xF0)
        {
            if (in.size() < 3) { in = {}; return U'\uFFFD'; }
            char32_t cp = static_cast<char32_t>((byte & 0x0F) << 12);
            cp |= static_cast<char32_t>((static_cast<uint8_t>(in[1]) & 0x3F) << 6);
            cp |= static_cast<char32_t>(static_cast<uint8_t>(in[2]) & 0x3F);
            in.remove_prefix(3);
            return cp;
        }
        else
        {
            if (in.size() < 4) { in = {}; return U'\uFFFD'; }
            char32_t cp = static_cast<char32_t>((byte & 0x07) << 18);
            cp |= static_cast<char32_t>((static_cast<uint8_t>(in[1]) & 0x3F) << 12);
            cp |= static_cast<char32_t>((static_cast<uint8_t>(in[2]) & 0x3F) << 6);
            cp |= static_cast<char32_t>(static_cast<uint8_t>(in[3]) & 0x3F);
            in.remove_prefix(4);
            return cp;
        }
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

    static void ensure_locale()
    {
        static const bool initialized = []() {
            std::setlocale(LC_ALL, "");
            return true;
        }();
        (void)initialized;
    }

    static bool is_zero_width(char32_t cp)
    {
        if (cp == 0 || cp == 0x200B || cp == 0x200C || cp == 0x200D) return true;
        if (cp >= 0xFE00 && cp <= 0xFE0F) return true; // Variation Selectors
        if (cp >= 0x0300 && cp <= 0x036F) return true; // Combining Diacritical Marks
        return false;
    }

    static bool is_unicode_wide(char32_t cp)
    {
        // CJK Unified Ideographs & Extensions
        if (cp >= 0x4E00 && cp <= 0x9FFF) return true;
        if (cp >= 0x3400 && cp <= 0x4DBF) return true;
        if (cp >= 0x20000 && cp <= 0x2FFFD) return true;
        if (cp >= 0x30000 && cp <= 0x3FFFD) return true;

        // Hiragana & Katakana
        if (cp >= 0x3040 && cp <= 0x309F) return true;
        if (cp >= 0x30A0 && cp <= 0x30FF) return true;

        // Hangul Syllables & Jamo
        if (cp >= 0xAC00 && cp <= 0xD7AF) return true;
        if (cp >= 0x1100 && cp <= 0x11FF) return true;

        // CJK Radicals, Symbols & Punctuation, Fullwidth forms
        if (cp >= 0x2E80 && cp <= 0x303E) return true;
        if (cp >= 0xFF01 && cp <= 0xFF60) return true;
        if (cp >= 0xFFE0 && cp <= 0xFFE6) return true;

        // Emojis and Pictographs (SMP plane)
        if (cp >= 0x1F300 && cp <= 0x1FAFF) return true;
        if (cp >= 0x1F000 && cp <= 0x1F02F) return true;
        if (cp >= 0x1F0A0 && cp <= 0x1F0FF) return true;

        return false;
    }

    int codepoint_width(char32_t cp)
    {   
        if (is_zero_width(cp))
        {
            return 0;
        }

        ensure_locale();
        int w = wcwidth(static_cast<wchar_t>(cp));
        if (w >= 0)
        {
            if (w == 1 && is_unicode_wide(cp))
            {
                return 2;
            }
            return w;
        }

        return is_unicode_wide(cp) ? 2 : 1;
    }

    int text_width(std::string_view text)
    {
        int width = 0;
        std::string_view rem = text;
        while (!rem.empty())
        {
            width += codepoint_width(next_codepoint(rem));
        }
        return width;
    }

}