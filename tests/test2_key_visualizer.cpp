#include <koto/koto.hpp>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using namespace koto;

static auto format_key_name(const KeyEvent& k) -> std::string
{
    std::string prefix;
    if (k.meta)  prefix += "Meta + ";
    if (k.ctrl)  prefix += "Ctrl + ";
    if (k.alt)   prefix += "Alt + ";
    if (k.shift && !k.is_char()) prefix += "Shift + ";

    std::string name;
    if (k.is(Key::Up))             name = "Up";
    else if (k.is(Key::Down))      name = "Down";
    else if (k.is(Key::Left))      name = "Left";
    else if (k.is(Key::Right))     name = "Right";
    else if (k.is(Key::Enter))     name = "Enter";
    else if (k.is(Key::Backspace)) name = "Backspace";
    else if (k.is(Key::Delete))    name = "Delete";
    else if (k.is(Key::Insert))    name = "Insert";
    else if (k.is(Key::Escape))    name = "Escape";
    else if (k.is(Key::Tab))       name = "Tab";
    else if (k.is(Key::BackTab))   name = "BackTab (Shift+Tab)";
    else if (k.is(Key::Home))      name = "Home";
    else if (k.is(Key::End))       name = "End";
    else if (k.is(Key::PageUp))    name = "PageUp";
    else if (k.is(Key::PageDown))  name = "PageDown";
    else if (k.is(Key::F1))        name = "F1";
    else if (k.is(Key::F2))        name = "F2";
    else if (k.is(Key::F3))        name = "F3";
    else if (k.is(Key::F4))        name = "F4";
    else if (k.is(Key::F5))        name = "F5";
    else if (k.is(Key::F6))        name = "F6";
    else if (k.is(Key::F7))        name = "F7";
    else if (k.is(Key::F8))        name = "F8";
    else if (k.is(Key::F9))        name = "F9";
    else if (k.is(Key::F10))       name = "F10";
    else if (k.is(Key::F11))       name = "F11";
    else if (k.is(Key::F12))       name = "F12";
    else if (k.is_char())
    {
        char utf8[5] = {};
        encode_utf8(k.ch, utf8);
        name = std::string(utf8);
    }
    else
    {
        std::stringstream ss;
        ss << "Special (0x" << std::hex << std::uppercase << k.ch << ")";
        name = ss.str();
    }

    return prefix + name;
}

int main()
{
    App app;

    std::string current_key_str = "Press any key...";
    std::string details_str = "Waiting for input";
    ListState history_state;

    app.on_key([&](const KeyEvent& key) {
        if (key.ctrl && (key.ch == 'q' || key.ch == 'Q'))
        {
            app.quit();
            return;
        }

        current_key_str = format_key_name(key);

        std::stringstream ss;
        ss << "Code: 0x" << std::hex << std::uppercase << key.ch
           << " | Ctrl: " << (key.ctrl ? "YES" : "no")
           << " | Alt: "  << (key.alt  ? "YES" : "no")
           << " | Shift: " << (key.shift ? "YES" : "no");
        details_str = ss.str();

        history_state.items.insert(history_state.items.begin(), current_key_str + " (" + details_str + ")");
        if (history_state.items.size() > 50)
        {
            history_state.items.pop_back();
        }
    });

    app.on_render([&](Frame& frame) {
        Area full = frame.area();
        if (full.width < 20 || full.height < 10) return;

        // Background
        Box().bg(Color::Rgb(17, 17, 17)).filled(true).draw(frame, full);

        // Vertical split: Header (3), Main (7), History (Fill), Footer (1)
        auto rows = Layout::vertical(full, {
            Constraint::fixed(3),
            Constraint::fixed(7),
            Constraint::fill(),
            Constraint::fixed(1),
        });

        // Header
        Box().border_style(BorderStyle::Rounded)
            .title(" koto :: Key Visualizer ")
            .title_align(TitleAlign::Left)
            .fg(Color::Cyan)
            .draw(frame, rows[0]);

        // Main display panel
        Area display_box = rows[1].inset(0, 2, 0, 2);
        Box().border_style(BorderStyle::Thick)
            .fg(Color::Emerald)
            .bg(Color::Rgb(30, 30, 30))
            .filled(true)
            .draw(frame, display_box);

        Area key_text_area = display_box.inset(1, 4, 1, 4);
        Text(current_key_str)
            .fg(Color::White)
            .bg(Color::Rgb(30, 30, 30))
            .draw(frame, key_text_area);

        Area details_area = display_box.inset(3, 4, 1, 4);
        Text(details_str)
            .fg(Color::Gray)
            .bg(Color::Rgb(30, 30, 30))
            .draw(frame, details_area);

        // History panel via List widget
        Area history_area = rows[2].inset(0, 2, 0, 2);
        List(history_state)
            .border_style(BorderStyle::Rounded)
            .title(" Key History ")
            .title_align(TitleAlign::Left)
            .fg(Color::LightGray)
            .highlight_symbol("▶ ")
            .highlight_fg(Color::Emerald)
            .overflow(TextOverflow::Ellipsis)
            .draw(frame, history_area);

        // Status bar
        Text("Press any key, function key (F1-F12), or modifier combination. Ctrl+Q to quit.")
            .fg(Color::DarkGray)
            .bg(Color::Rgb(17, 17, 17))
            .draw(frame, rows[3].inset(0, 2, 0, 0));
    });

    app.run();
    return 0;
}