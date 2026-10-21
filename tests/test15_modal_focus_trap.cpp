#include <koto/koto.hpp>
#include <string>
#include <vector>

using namespace koto;

int main()
{
    App app;
    FocusManager fm;

    // Background Workspace States
    ListState files_list({"src/main.cpp", "src/core.cpp", "src/auth.cpp", "include/koto.hpp", "Makefile"});
    files_list.set_selected(0);

    TextAreaState editor_state(
        "// Production Authentication Module\n"
        "#include <iostream>\n\n"
        "bool verify_credentials(const std::string& user, const std::string& token) {\n"
        "    // Press 'D' or click [ Delete File ] below to test Modal Focus Trapping!\n"
        "    return !user.empty() && token.length() >= 32;\n"
        "}"
    );

    const Id id_btn_delete("btn_open_delete_modal");
    const Id id_btn_about("btn_open_about_modal");

    // Modal state tracking
    enum class ActiveModal {
        None,
        DeleteConfirm,
        AboutDialog
    };
    ActiveModal active_modal = ActiveModal::None;

    // Modal inputs & buttons
    TextInputState delete_confirm_input;
    const Id id_modal_confirm("modal_btn_confirm");
    const Id id_modal_cancel("modal_btn_cancel");
    const Id id_modal_close_about("modal_btn_close_about");

    std::string toast_message = "Background workspace active. Press Tab to cycle, or 'D' to open delete modal.";

    auto close_modal = [&]() {
        if (active_modal != ActiveModal::None) {
            fm.pop_scope();
            active_modal = ActiveModal::None;
        }
    };

    auto open_delete_modal = [&]() {
        active_modal = ActiveModal::DeleteConfirm;
        delete_confirm_input.clear();
        fm.push_scope(Id("delete_modal"), ScopeMode::Modal);
        fm.add(delete_confirm_input)
          .add(id_modal_confirm, [&](const KeyEvent& k) {
              if (k.is(Key::Enter)) {
                  auto selected_file = files_list.selected_item().value_or("file");
                  toast_message = "Deleted file: " + std::string(selected_file);
                  close_modal();
                  return true;
              }
              return false;
          })
          .add(id_modal_cancel, [&](const KeyEvent& k) {
              if (k.is(Key::Enter)) {
                  toast_message = "Deletion canceled.";
                  close_modal();
                  return true;
              }
              return false;
          });
    };

    auto open_about_modal = [&]() {
        active_modal = ActiveModal::AboutDialog;
        fm.push_scope(Id("about_modal"), ScopeMode::Modal);
        fm.add(id_modal_close_about, [&](const KeyEvent& k) {
            if (k.is(Key::Enter)) {
                close_modal();
                return true;
            }
            return false;
        });
    };

    // Register Background Elements
    fm.add(files_list)
      .add(editor_state)
      .add(id_btn_delete, [&](const KeyEvent& k) {
          if (k.is(Key::Enter)) {
              open_delete_modal();
              return true;
          }
          return false;
      })
      .add(id_btn_about, [&](const KeyEvent& k) {
          if (k.is(Key::Enter)) {
              open_about_modal();
              return true;
          }
          return false;
      });

    app.use_focus(fm);

    app.on_key([&](const KeyEvent& key) {
        if (key.ctrl && (key.ch == 'q' || key.ch == 'Q')) {
            app.quit();
            return;
        }

        // Global modal trigger: 'd' or 'D' opens delete dialog
        if (active_modal == ActiveModal::None && (key.ch == 'd' || key.ch == 'D') && !fm.is_focused(editor_state)) {
            open_delete_modal();
            return;
        }

        // Escape always closes the active modal and restores focus
        if (active_modal != ActiveModal::None && key.is(Key::Escape)) {
            close_modal();
            toast_message = "Modal dismissed via Escape. Previous focus restored!";
            return;
        }

        // Modal Action: Enter on modal confirm button
        if (active_modal == ActiveModal::DeleteConfirm) {
            if (fm.is_focused(id_modal_confirm) && key.is(Key::Enter)) {
                auto selected_file = files_list.selected_item().value_or("file");
                toast_message = "Deleted file: " + std::string(selected_file);
                close_modal();
            } else if (fm.is_focused(id_modal_cancel) && key.is(Key::Enter)) {
                toast_message = "Deletion canceled.";
                close_modal();
            }
        } else if (active_modal == ActiveModal::AboutDialog) {
            if (fm.is_focused(id_modal_close_about) && key.is(Key::Enter)) {
                close_modal();
            }
        }
    });

    app.on_mouse([&](const MouseEvent& mouse) {
        if (mouse.action == MouseAction::Press && mouse.button == MouseButton::Left) {
            Id hit = app.frame().hit_at(mouse.row, mouse.col);

            if (active_modal == ActiveModal::None) {
                if (hit == id_btn_delete) {
                    open_delete_modal();
                } else if (hit == id_btn_about) {
                    open_about_modal();
                }
            } else if (active_modal == ActiveModal::DeleteConfirm) {
                if (hit == id_modal_confirm) {
                    auto selected_file = files_list.selected_item().value_or("file");
                    toast_message = "Deleted file: " + std::string(selected_file);
                    close_modal();
                } else if (hit == id_modal_cancel) {
                    toast_message = "Deletion canceled.";
                    close_modal();
                }
            } else if (active_modal == ActiveModal::AboutDialog) {
                if (hit == id_modal_close_about) {
                    close_modal();
                }
            }
        }
    });

    app.on_render([&](Frame& frame) {
        Area screen = frame.area();
        if (screen.width < 55 || screen.height < 18) {
            Box().title("Screen Too Small").draw(frame, screen);
            Text("Please enlarge your terminal window (min 55x18).").align(TextAlign::Center).draw(frame, Layout::center(screen, 46, 2));
            return;
        }

        // =====================================================================
        // 1. BACKGROUND LAYER (Dimmed if modal is active)
        // =====================================================================
        uint32_t bg_border_fg = (active_modal != ActiveModal::None) ? Color::DarkGray : COLOR_DEFAULT;
        uint32_t bg_title_fg  = (active_modal != ActiveModal::None) ? Color::DarkGray : Color::Cyan;

        auto v_splits = Layout::vertical(screen, {
            Constraint::fixed(3),
            Constraint::fill(),
            Constraint::fixed(3),
            Constraint::fixed(1)
        });

        // Top App Bar
        Box()
            .border_style(BorderStyle::Rounded)
            .fg(bg_border_fg)
            .title(" koto Focus Module — Modal Dialog & Focus Trapping Demo ")
            .title_fg(bg_title_fg)
            .title_align(TitleAlign::Left)
            .draw(frame, v_splits[0]);

        // Middle Split: Left Files List (30%) & Right Editor (70%)
        auto h_cols = Layout::horizontal(v_splits[1], {
            Constraint::percent(30),
            Constraint::fill()
        });

        List(files_list)
            .border_style(BorderStyle::Rounded)
            .fg(bg_border_fg)
            .title(" Project Files ")
            .highlight_symbol("▶ ")
            .highlight_fg(Color::Black)
            .highlight_bg(Color::Cyan)
            .focused_border_fg(Color::Cyan)
            .draw(frame, h_cols[0]);

        auto right_splits = Layout::vertical(h_cols[1], {
            Constraint::fill(),
            Constraint::fixed(3)
        });

        TextArea(editor_state)
            .border_style(BorderStyle::Rounded)
            .fg(bg_border_fg)
            .title(" Code Editor ")
            .title_align(TitleAlign::Center)
            .line_numbers(true)
            .focused_border_fg(Color::Cyan)
            .draw(frame, right_splits[0]);

        // Background Action Bar
        auto btn_row = Layout::horizontal(right_splits[1], {
            Constraint::fill(),
            Constraint::fill()
        });

        auto draw_bg_btn = [&](Area btn_area, Id id, std::string_view label, uint32_t glow_col) {
            bool is_foc = (active_modal == ActiveModal::None) && fm.is_focused(id);
            Box()
                .border_style(BorderStyle::Rounded)
                .fg(is_foc ? glow_col : (active_modal != ActiveModal::None ? Color::DarkGray : Color::LightGray))
                .bg(is_foc ? Color::DarkGray : COLOR_DEFAULT)
                .id(id)
                .focused(is_foc)
                .focused_border_fg(glow_col)
                .filled(true)
                .draw(frame, btn_area);

            Text(label)
                .fg(is_foc ? Color::White : (active_modal != ActiveModal::None ? Color::DarkGray : Color::White))
                .bg(is_foc ? Color::DarkGray : COLOR_DEFAULT)
                .align(TextAlign::Center)
                .draw(frame, btn_area.inset(1, 1, 1, 1));
        };

        draw_bg_btn(btn_row[0], id_btn_delete, "🗑 Delete File (Press 'D')", Color::Red);
        draw_bg_btn(btn_row[1], id_btn_about, "ℹ About koto Focus", Color::Cyan);

        // Status Toast Bar
        Box().border_style(BorderStyle::Single).fg(bg_border_fg).title("Status").draw(frame, v_splits[2]);
        Text(toast_message).fg(Color::Yellow).draw(frame, v_splits[2].inset(1, 2, 1, 2));

        Text(" [Tab/Shift+Tab] Cycle Focus  │  [Click] Hit-Test Focus  │  [Esc] Close Modal  │  [Ctrl+Q] Quit")
            .fg(Color::DarkGray)
            .align(TextAlign::Center)
            .draw(frame, v_splits[3]);

        // =====================================================================
        // 2. MODAL LAYER (Focus Trapped & Centered via Layout::center)
        // =====================================================================
        if (active_modal == ActiveModal::DeleteConfirm) {
            Area modal_area = Layout::center(screen, 50, 11);

            // Modal Background Backdrop Box
            Box()
                .border_style(BorderStyle::Double)
                .fg(Color::Red)
                .bg(Color::Black)
                .title(" ⚠ CONFIRM DELETION ⚠ ")
                .title_fg(Color::Red)
                .title_align(TitleAlign::Center)
                .filled(true)
                .draw(frame, modal_area);

            Area inner = modal_area.inset(1, 2, 1, 2);
            auto modal_v = Layout::vertical(inner, {
                Constraint::fixed(2), // Warning text
                Constraint::fixed(3), // Input confirmation
                Constraint::fixed(3)  // Confirm / Cancel buttons
            });

            std::string prompt = "Permanently delete '" + std::string(files_list.selected_item().value_or("selected")) + "'?\nFocus is trapped inside this modal!";
            Text(prompt).fg(Color::White).bg(Color::Black).draw(frame, modal_v[0]);

            TextInput(delete_confirm_input)
                .border_style(BorderStyle::Rounded)
                .title("Type 'DELETE' to confirm")
                .placeholder("Type here...")
                .focused_border_fg(Color::Red)
                .bg(Color::Black)
                .draw(frame, modal_v[1]);

            auto modal_btns = Layout::horizontal(modal_v[2], {
                Constraint::fill(),
                Constraint::fill()
            });

            auto draw_modal_btn = [&](Area btn_area, Id id, std::string_view label, uint32_t active_col) {
                bool is_foc = fm.is_focused(id);
                Box()
                    .border_style(BorderStyle::Rounded)
                    .fg(is_foc ? active_col : Color::DarkGray)
                    .bg(is_foc ? Color::DarkGray : Color::Black)
                    .id(id)
                    .focused(is_foc)
                    .focused_border_fg(active_col)
                    .filled(true)
                    .draw(frame, btn_area);

                Text(label)
                    .fg(is_foc ? Color::White : Color::LightGray)
                    .bg(is_foc ? Color::DarkGray : Color::Black)
                    .align(TextAlign::Center)
                    .draw(frame, btn_area.inset(1, 1, 1, 1));
            };

            draw_modal_btn(modal_btns[0], id_modal_confirm, "✔ Confirm Delete", Color::Red);
            draw_modal_btn(modal_btns[1], id_modal_cancel, "✖ Cancel [Esc]", Color::Cyan);

        } else if (active_modal == ActiveModal::AboutDialog) {
            Area modal_area = Layout::center(screen, 48, 10);

            Box()
                .border_style(BorderStyle::Thick)
                .fg(Color::Cyan)
                .bg(Color::Black)
                .title(" About koto Focus System ")
                .title_fg(Color::Cyan)
                .title_align(TitleAlign::Center)
                .filled(true)
                .draw(frame, modal_area);

            Area inner = modal_area.inset(1, 2, 1, 2);
            auto modal_v = Layout::vertical(inner, {
                Constraint::fixed(4), // Info text
                Constraint::fixed(3)  // Close button
            });

            std::string about_text = 
                "• 1D Tab Ring & 2D Spatial Arrow Navigation\n"
                "• SGR Mouse Click Hit-Testing (Decoded via Frame)\n"
                "• Modal Scoping & Shielded Background Click Absorbtion\n"
                "• RAII IdScope Automatic Focus Restoration";

            Text(about_text).fg(Color::White).bg(Color::Black).draw(frame, modal_v[0]);

            bool is_foc = fm.is_focused(id_modal_close_about);
            Box()
                .border_style(BorderStyle::Rounded)
                .fg(is_foc ? Color::Cyan : Color::DarkGray)
                .bg(is_foc ? Color::DarkGray : Color::Black)
                .id(id_modal_close_about)
                .focused(is_foc)
                .focused_border_fg(Color::Cyan)
                .filled(true)
                .draw(frame, modal_v[1]);

            Text("Close Dialog [Enter / Esc]")
                .fg(is_foc ? Color::White : Color::LightGray)
                .bg(is_foc ? Color::DarkGray : Color::Black)
                .align(TextAlign::Center)
                .draw(frame, modal_v[1].inset(1, 1, 1, 1));
        }
    });

    app.run();
    return 0;
}
