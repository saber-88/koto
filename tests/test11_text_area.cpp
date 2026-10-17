#include <koto/koto.hpp>
#include <string>
#include <vector>

using namespace koto;

int main()
{
    App app;

    std::string sample_code = 
        "// Welcome to koto TextArea interactive demo!\n"
        "// You have full multi-line editing with line numbers & scrolling.\n"
        "\n"
        "#include <iostream>\n"
        "#include <vector>\n"
        "\n"
        "int main() {\n"
        "    std::cout << \"Hello, koto!\" << std::endl;\n"
        "    // Try moving up and down across lines of varying lengths\n"
        "    // Notice how Sticky cursor mode preserves your goal column!\n"
        "    std::vector<int> numbers = {1, 2, 3, 4, 5};\n"
        "    for (int n : numbers) {\n"
        "        std::cout << n << \"\\n\";\n"
        "    }\n"
        "    return 0;\n"
        "}";

    TextAreaState editor_state(sample_code);

    app.on_key([&](const KeyEvent& key) {
        if (key.is(Key::Escape) || (key.ctrl && (key.ch == 'C' || key.ch == 'c')))
        {
            app.quit();
            return;
        }

        // F2: Toggle Line Numbers
        if (key.is(Key::F2))
        {
            editor_state.show_line_number = !editor_state.show_line_number;
            return;
        }

        // F3: Toggle CursorColumnMode (Sticky vs Clamp)
        if (key.is(Key::F3))
        {
            editor_state.cursor_mode = (editor_state.cursor_mode == CursorColumnMode::Sticky)
                ? CursorColumnMode::Clamp
                : CursorColumnMode::Sticky;
            return;
        }

        // F4: Toggle ScrollMode (Margin vs Clamp)
        if (key.is(Key::F4))
        {
            editor_state.scroll_mode = (editor_state.scroll_mode == ScrollMode::Margin)
                ? ScrollMode::Clamp
                : ScrollMode::Margin;
            return;
        }

        // Pass key to editor
        editor_state.handle_event(key);
    });

    app.on_render([&](Frame& frame) {
        Area screen = frame.area();
        if (!screen.is_valid())
        {
            return;
        }

        // Layout: Main Editor (Flex 1) + Status Bar (Fixed 1) + Help Bar (Fixed 1)
        auto chunks = Layout::vertical(screen, {
            Constraint::fill(),
            Constraint::fixed(1),
            Constraint::fixed(1)
        });

        // 1. Draw TextArea
        std::string mode_title = " Code Editor [" + 
            std::string(editor_state.cursor_mode == CursorColumnMode::Sticky ? "Sticky" : "Clamp") + " Cursor, " +
            std::string(editor_state.scroll_mode == ScrollMode::Margin ? "Margin" : "Clamp") + " Scroll] ";

        TextArea(editor_state)
            .border_style(BorderStyle::Rounded)
            .title(mode_title)
            .title_align(TitleAlign::Left)
            .line_numbers(editor_state.show_line_number)
            .line_number_fg(Color::DarkGray)
            .focused(true)
            .draw(frame, chunks[0]);

        // 2. Draw Status Bar
        std::string status = " Ln " + std::to_string(editor_state.cursor_row + 1) + 
            "/" + std::to_string(editor_state.text.size()) + 
            ", Col " + std::to_string(editor_state.cursor_col) + 
            " (Goal: " + std::to_string(editor_state.preferred_col) + ")" +
            " | Viewport: Row " + std::to_string(editor_state.scroll_row) + 
            ", Col " + std::to_string(editor_state.scroll_col);

        Text(status)
            .fg(Color::Black)
            .bg(Color::Cyan)
            .align(TextAlign::Left)
            .draw(frame, chunks[1]);

        // 3. Draw Help / Shortcut Bar
        std::string help = " [F2] Toggle Line Numbers  [F3] Sticky/Clamp Cursor  [F4] Margin/Clamp Scroll  [Esc] Quit";
        Text(help)
            .fg(Color::White)
            .bg(Color::Rgb(40, 44, 52))
            .align(TextAlign::Left)
            .draw(frame, chunks[2]);
    });

    app.run();
    return 0;
}
