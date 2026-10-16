#include <koto/koto.hpp>
#include <string>
#include <vector>

using namespace koto;

int main()
{
    App app;

    // We maintain a list of logged activity across all 3 inputs directly in ListState
    ListState list_state({
        "Welcome to the koto TextInput 3-layer demo!",
        "Press Tab / Shift+Tab to switch between the 3 input fields.",
    });

    auto add_log = [&](const std::string& msg) {
        list_state.items.push_back(msg);
        list_state.selected = static_cast<int>(list_state.items.size()) - 1;
    };

    // =========================================================================
    // LAYER 1: Pre-Hook Interceptor (.on_key)
    // Customizes specific key actions (Enter to submit, Esc to clear)
    // while keeping default typing and arrow navigation intact.
    // =========================================================================
    TextInputState input_layer1;
    input_layer1.on_key([&](TextInputState& s, const KeyEvent& k) {
        if (k.is(Key::Enter))
        {
            if (!s.text().empty())
            {
                add_log("[Layer 1 Submitted] " + std::string(s.text()));
                s.clear();
                return true; // Event consumed!
            }
        }
        if (k.is(Key::Escape))
        {
            s.clear();
            add_log("[Layer 1] Cleared via Escape");
            return true; // Event consumed!
        }
        return false; // Pass through to default editing (arrows, typing, backspace)
    });

    // =========================================================================
    // LAYER 2: Validation / Input Filter Predicate (.set_filter)
    // Restricts what characters are allowed into the buffer.
    // Here: Digits only (0-9) and max length 5 (e.g. port number 0..65535).
    // =========================================================================
    TextInputState input_layer2;
    input_layer2.set_filter([](std::string_view current, char32_t cp) {
        bool is_digit = (cp >= U'0' && cp <= U'9');
        bool within_limit = current.size() < 5;
        return is_digit && within_limit;
    });
    input_layer2.on_key([&](TextInputState& s, const KeyEvent& k) {
        if (k.is(Key::Enter) && !s.text().empty())
        {
            add_log("[Layer 2 Port Selected] :" + std::string(s.text()));
            return true;
        }
        return false;
    });

    // =========================================================================
    // LAYER 3: Granular Primitives / Modal Vim Mode
    // Uses low-level primitive mutations (.move_left, .delete_forward, etc.)
    // to build a custom modal state machine (Normal Mode vs Insert Mode).
    // =========================================================================
    TextInputState input_layer3("edit me with vim keys");
    bool vim_insert_mode = false;

    input_layer3.on_key([&](TextInputState& s, const KeyEvent& k) {
        if (!vim_insert_mode)
        {
            // --- VIM NORMAL MODE ---
            if (k.ch == 'i' || k.ch == 'I')
            {
                vim_insert_mode = true;
                add_log("[Layer 3] Switched to -- INSERT MODE --");
                return true;
            }
            if (k.ch == 'h') { s.move_left(); return true; }
            if (k.ch == 'l') { s.move_right(); return true; }
            if (k.ch == '0') { s.move_home(); return true; }
            if (k.ch == '$') { s.move_end(); return true; }
            if (k.ch == 'x') { s.delete_forward(); return true; }
            if (k.ch == 'd' || (k.ctrl && (k.ch == 'U' || k.ch == 'u')))
            {
                s.clear();
                return true;
            }
            // In normal mode, consume all other printable/unhandled keys so they are NEVER typed into text
            return true;
        }
        else
        {
            // --- VIM INSERT MODE ---
            if (k.is(Key::Escape))
            {
                vim_insert_mode = false;
                add_log("[Layer 3] Switched to -- NORMAL MODE --");
                return true;
            }
            return false; // Allow standard character insertion & editing
        }
    });

    // Focus management across the 3 fields
    int active_focus = 0;
    auto update_focus = [&]() {
        input_layer1.focused = (active_focus == 0);
        input_layer2.focused = (active_focus == 1);
        input_layer3.focused = (active_focus == 2);
    };
    update_focus();

    app.on_key([&](const KeyEvent& key) {
        // Tab / Shift+Tab: cycle focus across the 3 inputs
        if (key.is(Key::BackTab))
        {
            active_focus = (active_focus + 2) % 3;
            update_focus();
            return;
        }
        if (key.is(Key::Tab))
        {
            active_focus = (active_focus + 1) % 3;
            update_focus();
            return;
        }

        // Route key event to the currently focused input
        if (active_focus == 0)      input_layer1.handle_event(key);
        else if (active_focus == 1) input_layer2.handle_event(key);
        else if (active_focus == 2) input_layer3.handle_event(key);
    });

    app.on_render([&](Frame& frame) {
        Area area = frame.area();
        if (area.width < 30 || area.height < 16) return;

        // Layout split
        auto sections = Layout::vertical(area, {
            Constraint::fixed(3), // Header
            Constraint::fixed(3), // Layer 1 (Interceptor)
            Constraint::fixed(3), // Layer 2 (Filter)
            Constraint::fixed(3), // Layer 3 (Vim Primitives)
            Constraint::fixed(1), // Activity title
            Constraint::fill(),   // Activity list
            Constraint::fixed(1), // Footer
        });

        // 1. Header Box
        Box()
            .border_style(BorderStyle::Rounded)
            .fg("#00AAFF")
            .title(" koto — 3 Layers of TextInput Customization ")
            .title_fg("#FFFFFF")
            .draw(frame, sections[0]);

        // 2. Layer 1: Interceptor Input
        TextInput(input_layer1)
            .placeholder("Layer 1: Enter submits, Esc clears...")
            .border_style(BorderStyle::Rounded)
            .focused_border_fg("#00FF88")
            .fg("#FFFFFF")
            .draw(frame, sections[1]);

        // 3. Layer 2: Filtered Numeric Input
        TextInput(input_layer2)
            .placeholder("Layer 2: Digits only (max 5 chars)...")
            .border_style(BorderStyle::Rounded)
            .focused_border_fg("#FFAA00")
            .fg("#FFFFFF")
            .draw(frame, sections[2]);

        // 4. Layer 3: Vim Modal Input
        std::string vim_mode_title = vim_insert_mode ? " Vim: [INSERT MODE] " : " Vim: [NORMAL MODE - h/l/x/0/$/i] ";

        TextInput(input_layer3)
            .title(vim_mode_title)
            .placeholder("Layer 3: Vim primitives...")
            .border_style(BorderStyle::Rounded)
            .focused_border_fg(vim_insert_mode ? "#00FF88" : "#FF55AA")
            .fg("#FFFFFF")
            .draw(frame, sections[3]);

        // 5. Activity Log Label
        Text("Activity & Event Log:")
            .fg("#888888")
            .draw(frame, sections[4].inset(0, 1, 0, 0));

        // 6. Activity Log List
        List(list_state)
            .border_style(BorderStyle::Single)
            .fg("#CCCCCC")
            .highlight_fg("#00FFAA")
            .draw(frame, sections[5]);

        // 7. Footer Status Bar
        Text("[Tab / Shift+Tab] Switch Field | [Ctrl+Q] Quit")
            .fg("#666666")
            .draw(frame, sections[6].inset(0, 1, 0, 0));
    });

    app.run();
    return 0;
}
