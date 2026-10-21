#include <koto/koto.hpp>
#include <string>
#include <vector>

using namespace koto;

int main()
{
    App app;
    FocusManager fm;

    // Form States
    TabsState category_tabs({"1. Personal", "2. Account", "3. Preferences"});
    TextInputState username_in("alex_mercer");
    TextInputState email_in("alex@example.com");
    ListState role_list({"Senior Developer", "System Architect", "DevOps Engineer", "UI/UX Designer"});
    role_list.set_selected(0);
    
    TextAreaState bio_area(
        "// Staff Software Engineer\n"
        "// Passionate about systems programming,\n"
        "// Modern C++23, terminal user interfaces,\n"
        "// and high-performance low-latency architectures."
    );

    std::string status_message = "Ready. Use Tab / Shift+Tab to navigate, or click any widget.";
    std::string last_mouse_click = "None";

    // Action button IDs
    const Id id_btn_save("btn_save");
    const Id id_btn_reset("btn_reset");
    const Id id_btn_exit("btn_exit");

    // Register all focusable widgets and buttons into FocusManager
    fm.add(category_tabs)
      .add(username_in)
      .add(email_in)
      .add(role_list)
      .add(bio_area)
      .add(id_btn_save, [&](const KeyEvent& k) {
          if (k.is(Key::Enter)) {
              status_message = "Saved profile for @" + std::string(username_in.text()) + " (" + std::string(email_in.text()) + ")!";
              return true;
          }
          return false;
      })
      .add(id_btn_reset, [&](const KeyEvent& k) {
          if (k.is(Key::Enter)) {
              username_in.set_text("alex_mercer");
              email_in.set_text("alex@example.com");
              role_list.set_selected(0);
              status_message = "Fields reset to initial defaults.";
              return true;
          }
          return false;
      })
      .add(id_btn_exit, [&](const KeyEvent& k) {
          if (k.is(Key::Enter)) {
              app.quit();
              return true;
          }
          return false;
      });

    // Connect FocusManager to App
    app.use_focus(fm);

    app.on_mouse([&](const MouseEvent& mouse) {
        if (mouse.action == MouseAction::Press && mouse.button == MouseButton::Left)
        {
            last_mouse_click = "Row: " + std::to_string(mouse.row) + ", Col: " + std::to_string(mouse.col);
            
            // Check if user clicked directly on action buttons
            Id clicked_id = app.frame().hit_at(mouse.row, mouse.col);
            if (clicked_id == id_btn_save) {
                status_message = "Clicked [ Save Profile ] — Profile saved!";
            } else if (clicked_id == id_btn_reset) {
                username_in.set_text("alex_mercer");
                email_in.set_text("alex@example.com");
                role_list.set_selected(0);
                status_message = "Clicked [ Reset ] — Fields reset.";
            } else if (clicked_id == id_btn_exit) {
                app.quit();
            }
        }
    });

    app.on_key([&](const KeyEvent& key) {
        if (key.ctrl && (key.ch == 'q' || key.ch == 'Q')) {
            app.quit();
            return;
        }

        // Global shortcuts when not typing
        if (key.is(Key::F5)) {
            status_message = "Profile reloaded from disk.";
        }
    });

    app.on_render([&](Frame& frame) {
        Area screen = frame.area();
        if (screen.width < 50 || screen.height < 18) {
            Box().title("Screen Too Small").draw(frame, screen);
            Text("Please enlarge your terminal window (min 50x18).").align(TextAlign::Center).draw(frame, Layout::center(screen, 46, 2));
            return;
        }

        // Vertical split: Header (3) -> Body (Fill) -> Status (3) -> Footer (1)
        auto v_splits = Layout::vertical(screen, {
            Constraint::fixed(3),
            Constraint::fill(),
            Constraint::fixed(3),
            Constraint::fixed(1)
        });

        // 1. Header Box
        Box()
            .border_style(BorderStyle::Rounded)
            .fg(Color::Cyan)
            .title(" koto Focus Module — Interactive Multi-Widget Form Demo ")
            .title_align(TitleAlign::Left)
            .draw(frame, v_splits[0]);

        // 2. Body split: Left Column (40%) and Right Column (60%)
        auto h_cols = Layout::horizontal(v_splits[1], {
            Constraint::percent(45),
            Constraint::fill()
        });

        // Left Column components (Tabs -> Username -> Email -> Role List)
        auto left_rows = Layout::vertical(h_cols[0], {
            Constraint::fixed(3), // Tabs
            Constraint::fixed(3), // Username
            Constraint::fixed(3), // Email
            Constraint::fill()    // Role List
        });

        // Category Tabs
        Tabs(category_tabs)
            .border_style(BorderStyle::Rounded)
            .title("Section")
            .style(TabsStyle::Pill)
            .focused_border_fg(Color::Cyan)
            .draw(frame, left_rows[0]);

        // Username Input
        TextInput(username_in)
            .border_style(BorderStyle::Rounded)
            .title("Username")
            .placeholder("Enter username...")
            .focused_border_fg(Color::Cyan)
            .draw(frame, left_rows[1]);

        // Email Input
        TextInput(email_in)
            .border_style(BorderStyle::Rounded)
            .title("Email Address")
            .placeholder("Enter email address...")
            .focused_border_fg(Color::Cyan)
            .draw(frame, left_rows[2]);

        // Role Selection List
        List(role_list)
            .border_style(BorderStyle::Rounded)
            .title("Select Role (Up/Down or j/k)")
            .highlight_symbol("▶ ")
            .highlight_fg(Color::Black)
            .highlight_bg(Color::Cyan)
            .focused_border_fg(Color::Cyan)
            .draw(frame, left_rows[3]);

        // Right Column: TextArea Editor (Top) & Action Buttons (Bottom)
        auto right_rows = Layout::vertical(h_cols[1], {
            Constraint::fill(),   // Bio TextArea
            Constraint::fixed(3)  // Action Buttons
        });

        TextArea(bio_area)
            .border_style(BorderStyle::Rounded)
            .title("User Bio & Notes (Multi-Line Editor)")
            .title_align(TitleAlign::Center)
            .line_numbers(true)
            .focused_border_fg(Color::Cyan)
            .draw(frame, right_rows[0]);

        // 3 Action Buttons in a horizontal row
        auto btn_cols = Layout::horizontal(right_rows[1], {
            Constraint::fill(),
            Constraint::fill(),
            Constraint::fill()
        });

        auto draw_button = [&](Area btn_area, Id id, std::string_view label, uint32_t active_col) {
            bool is_foc = fm.is_focused(id);
            uint32_t border_col = is_foc ? active_col : Color::DarkGray;
            uint32_t text_col = is_foc ? Color::White : Color::LightGray;

            Box()
                .border_style(BorderStyle::Rounded)
                .fg(border_col)
                .bg(is_foc ? Color::DarkGray : COLOR_DEFAULT)
                .id(id)
                .focused(is_foc)
                .focused_border_fg(active_col)
                .filled(true)
                .draw(frame, btn_area);

            Text(label)
                .fg(text_col)
                .bg(is_foc ? Color::DarkGray : COLOR_DEFAULT)
                .align(TextAlign::Center)
                .draw(frame, btn_area.inset(1, 1, 1, 1));
        };

        draw_button(btn_cols[0], id_btn_save, "✔ Save Profile", Color::Emerald);
        draw_button(btn_cols[1], id_btn_reset, "↺ Reset", Color::Yellow);
        draw_button(btn_cols[2], id_btn_exit, "✖ Exit", Color::Red);

        // 3. Status Box
        Box()
            .border_style(BorderStyle::Single)
            .title("Focus Inspector")
            .draw(frame, v_splits[2]);

        Area status_area = v_splits[2].inset(1, 2, 1, 2);
        std::string focus_desc;
        if (fm.is_focused(category_tabs)) focus_desc = "Tabs [Section]";
        else if (fm.is_focused(username_in)) focus_desc = "TextInput [Username]";
        else if (fm.is_focused(email_in)) focus_desc = "TextInput [Email]";
        else if (fm.is_focused(role_list)) focus_desc = "List [Role Selection]";
        else if (fm.is_focused(bio_area)) focus_desc = "TextArea [Bio & Notes]";
        else if (fm.is_focused(id_btn_save)) focus_desc = "Button [Save Profile]";
        else if (fm.is_focused(id_btn_reset)) focus_desc = "Button [Reset]";
        else if (fm.is_focused(id_btn_exit)) focus_desc = "Button [Exit]";
        else focus_desc = "None";

        std::string inspector_text = "Active Target: " + focus_desc + 
                                     "  (Index " + std::to_string(fm.current_index() + 1) + "/" + std::to_string(fm.size()) + ")" +
                                     "  │  Last Click: " + last_mouse_click +
                                     "  │  " + status_message;

        Text(inspector_text).fg(Color::Yellow).draw(frame, status_area);

        // 4. Footer Help Line
        Text(" [Tab/Shift+Tab] Cycle Focus  │  [Click] Direct Hit-Focus  │  [Enter] Trigger  │  [Ctrl+Q] Quit")
            .fg(Color::DarkGray)
            .align(TextAlign::Center)
            .draw(frame, v_splits[3]);
    });

    app.run();
    return 0;
}
