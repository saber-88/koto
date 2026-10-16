#pragma once

#include <string_view>
namespace koto
{

    char32_t next_codepoint(const char*& ptr, const char* end);
    int encode_utf8(char32_t code , char*);

    int codepoint_width(char32_t cp);
    int text_width(std::string_view text);
}