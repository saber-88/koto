#pragma once

#include "koto/event.hpp"
#include "koto/widget.hpp"
#include "koto/box.hpp"
#include "koto/color.hpp"
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace koto
{
    enum class CursorColumnMode
    {
        Clamp,
        Sticky
    };

    enum class ScrollMode
    {
        Clamp,
        Margin
    };

    struct TextAreaState
    {
        std::vector<std::string> text {""};
        size_t cursor_row = 0;
        size_t cursor_col = 0;
        size_t preferred_col = 0;
        size_t scroll_row = 0;
        size_t scroll_col = 0;
        size_t scroll_margin = 1;
        bool show_line_number = true;

        CursorColumnMode cursor_mode = CursorColumnMode::Sticky;
        ScrollMode scroll_mode = ScrollMode::Margin;

        TextAreaState();
        TextAreaState(std::string_view initial_text);
        TextAreaState(std::vector<std::string> initial_lines);

        void insert(char32_t cp);
        void insert(std::string_view str);
        void newline();
        void backspace();
        void delete_forward();

        void move_left();
        void move_right();
        void move_up();
        void move_down();
        void move_home();
        void move_end();

        void clear();
        void set_text(std::string_view new_text);
        std::string to_string() const;

        bool handle_event(const KeyEvent& key);
    };

    class TextArea : public Widget
    {
    public:
        TextArea(TextAreaState& state);

        TextArea& border_style(BorderStyle style);
        TextArea& title(std::string_view title);
        TextArea& title_align(TitleAlign align);
        TextArea& line_numbers(bool show);
        TextArea& fg(std::string_view fg);
        TextArea& fg(uint32_t fg);
        TextArea& bg(std::string_view bg);
        TextArea& bg(uint32_t bg);
        TextArea& line_number_fg(std::string_view fg);
        TextArea& line_number_fg(uint32_t fg);
        TextArea& focused(bool f);
        TextArea& cursor_mode(CursorColumnMode mode);
        TextArea& scroll_mode(ScrollMode mode, size_t margin = 1);

        void draw(Frame& frame, Area area) override;

    private:
        TextAreaState& m_state;
        BorderStyle m_border_style = BorderStyle::Single;
        std::string m_title;
        TitleAlign m_title_align = TitleAlign::Left;
        bool m_show_line_numbers = true;
        uint32_t m_fg = COLOR_DEFAULT;
        uint32_t m_bg = COLOR_DEFAULT;
        uint32_t m_line_number_fg = Color::DarkGray;
        bool m_focused = true;
    };
}