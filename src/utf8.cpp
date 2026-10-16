#include "koto/utf8.hpp"
#include <cstdint>
#include <string_view>
#include <wchar.h>


namespace koto
{
    char32_t next_codepoint(const char*& ptr, const char* end)
    {
        if (ptr >= end) return 0;

        uint8_t byte = static_cast<uint8_t>(*ptr);

        auto remaining = [&]() { return static_cast<int>(end - ptr); };

        if (byte < 0x80)
        {
            ptr += 1;
            return byte;
        }
        else if (byte < 0xE0)
        {
            if (remaining() < 2) { ptr = end; return U'\uFFFD'; }
            char32_t cp = static_cast<char32_t>((byte & 0x1F) << 6);
            cp |= (static_cast<uint8_t>(ptr[1]) & 0x3F);
            ptr += 2;
            return cp;
        }
        else if (byte < 0xF0)
        {
            if (remaining() < 3) { ptr = end; return U'\uFFFD'; }
            char32_t cp = static_cast<char32_t>((byte & 0x0F) << 12);
            cp |= static_cast<char32_t>((static_cast<uint8_t>(ptr[1]) & 0x3F) << 6);
            cp |= static_cast<char32_t>(static_cast<uint8_t>(ptr[2]) & 0x3F);
            ptr += 3;
            return cp;
        }
        else
        {
            if (remaining() < 4) { ptr = end; return U'\uFFFD'; }
            char32_t cp = static_cast<char32_t>((byte & 0x07) << 18);
            cp |= static_cast<char32_t>((static_cast<uint8_t>(ptr[1]) & 0x3F) << 12);
            cp |= static_cast<char32_t>((static_cast<uint8_t>(ptr[2]) & 0x3F) << 6);
            cp |= static_cast<char32_t>(static_cast<uint8_t>(ptr[3]) & 0x3F);
            ptr += 4;
            return cp;
        }
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

    int codepoint_width(char32_t cp)
    {   
        int w =  wcwidth(static_cast<wchar_t>(cp));
        return (w < 0) ? 1 : w;
    }

    int text_width(std::string_view text)
    {
        int width = 0;

        const char* ptr = text.data();
        const char* end = ptr + text.size();

        while(ptr < end)
        {
            width += codepoint_width(next_codepoint(ptr, end));
        }
        return width;
    }

}