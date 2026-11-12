---
title: Modal Dialogs and Focus Trapping
description: How to trap keyboard focus, shield background mouse clicks, and restore prior focus using FocusManager modal scopes
sidebar:
  label: Modal Dialogs
  order: 4
---

## Goal

Open a centered modal confirmation dialog that traps `Tab`, `Shift+Tab`, and 2D spatial arrow navigation inside the dialog, shields background widgets from mouse clicks, and restores focus to the previously active background widget when dismissed.

<!-- src: include/koto/focus.hpp:79-93, 281-295 -->
<!-- src: src/focus.cpp:717-777 -->

## Complete working example

```cpp
#include <koto/koto.hpp>
#include <cstdio>
#include <string>

using namespace koto;

int main()
{
    App app;
    FocusManager fm;

    ListState files({"main.cpp", "CMakeLists.txt", "README.md"});
    TextInputState confirm_input;
    const Id id_open_modal("btn_open_modal");
    const Id id_confirm("modal_confirm");
    const Id id_cancel("modal_cancel");

    bool modal_open = false;
    std::string status = "Press D or click Delete to open modal. Press Ctrl+Q to quit.";

    auto close_modal = [&]() {
        if (modal_open) {
            fm.pop_scope();
            modal_open = false;
        }
    };

    auto open_modal = [&]() {
        if (modal_open) return;
        modal_open = true;
        confirm_input.clear();
        fm.push_scope(Id("delete_modal"), ScopeMode::Modal);
        fm.add(confirm_input)
          .add(id_confirm, [&](const KeyEvent& k) {
              if (k.is(Key::Enter)) {
                  status = "Confirmed deletion.";
                  close_modal();
                  return true;
              }
              return false;
          })
          .add(id_cancel, [&](const KeyEvent& k) {
              if (k.is(Key::Enter)) {
                  status = "Canceled deletion.";
                  close_modal();
                  return true;
              }
              return false;
          });
    };

    fm.add(files)
      .add(id_open_modal, [&](const KeyEvent& k) {
          if (k.is(Key::Enter) || k.ch == ' ') {
              open_modal();
              return true;
          }
          return false;
      });

    app.use_focus(fm);

    app.on_key([&](const KeyEvent& key) {
        if (key.ctrl && key.ch == 'Q') {
            app.quit();
            return;
        }
        if (modal_open && key.is(Key::Escape)) {
            status = "Modal dismissed with Escape.";
            close_modal();
            return;
        }
        if (!modal_open && (key.ch == 'd' || key.ch == 'D')) {
            open_modal();
        }
    });

    app.on_render([&](Frame& frame) {
        auto [body_area, btn_area, footer_area] = Layout::vertical(frame.area(), {
            Constraint::fill(),
            Constraint::fixed(3),
            Constraint::fixed(1),
        });

        List(files)
            .title("Files")
            .border_style(BorderStyle::Rounded)
            .highlight_symbol("> ")
            .highlight_fg(Color::Emerald)
            .draw(frame, body_area);

        Button("Delete Selected File (D)")
            .id(id_open_modal)
            .focused(!modal_open && fm.is_focused(id_open_modal))
            .draw(frame, btn_area);

        Text(status).fg(Color::DarkGray).draw(frame, footer_area);

        if (modal_open) {
            Area dialog_area = Layout::center(frame.area(), 46, 9);
            Box()
                .title("Confirm Delete")
                .border_style(BorderStyle::Double)
                .fg(Color::Red)
                .bg(Color::Black)
                .filled(true)
                .draw(frame, dialog_area);

            Area inner = dialog_area.inset(1, 2, 1, 2);
            auto [input_row, action_row] = Layout::vertical(inner, {
                Constraint::fixed(3),
                Constraint::fixed(3),
            });

            TextInput(confirm_input)
                .title("Note")
                .border_style(BorderStyle::Rounded)
                .bg(Color::Black)
                .draw(frame, input_row);

            auto [ok_area, cancel_area] = Layout::horizontal(action_row, {
                Constraint::fill(),
                Constraint::fill(),
            });

            Button("Confirm")
                .id(fm.id(id_confirm))
                .focused(fm.is_focused(id_confirm))
                .focused_border_fg(Color::Red)
                .bg(Color::Black)
                .draw(frame, ok_area);

            Button("Cancel")
                .id(fm.id(id_cancel))
                .focused(fm.is_focused(id_cancel))
                .focused_border_fg(Color::Cyan)
                .bg(Color::Black)
                .draw(frame, cancel_area);
        }
    });

    if (auto res = app.run(); !res) {
        std::fprintf(stderr, "Error: %.*s\n",
                     static_cast<int>(res.error().message().size()),
                     res.error().message().data());
        return 1;
    }
    return 0;
}
```

## Step-by-step explanation

1. **Opening the modal (`fm.push_scope(Id("delete_modal"), ScopeMode::Modal)`)**:
   - `push_scope` saves the currently focused background `Id` onto `m_saved_focus_stack` and pushes `{scope_id, ScopeMode::Modal}` onto `m_scope_stack` (`src/focus.cpp:722-742`).
   - Registering `confirm_input`, `id_confirm`, and `id_cancel` while the modal scope is on the stack stamps each `FocusTarget` with `.modal_scope = Id("delete_modal")` and `.id = fm.scoped_id(base_id)`, and immediately shifts focus into the first modal target (`src/focus.cpp:104-115`).
2. **Keeping the modal scope active across frames**:
   - Because `App::dispatch_event` runs outside `on_render`, the modal scope must remain on `FocusManager`'s stack for the entire duration the dialog is open (`open_modal()` pushes it; `close_modal()` pops it).
   - While `active_modal_scope()` is `Id("delete_modal")`, `FocusManager::handle_key` (`Tab`, `BackTab`, and spatial arrows) only visits targets whose `modal_scope == Id("delete_modal")`, and `FocusManager::handle_mouse` absorbs any clicks on `files` or `id_open_modal` (`src/focus.cpp:642-660`).
3. **Tagging stateless modal widgets with `fm.id(...)`**:
   - When `fm.add(id_confirm, ...)` runs inside `delete_modal`, the stored target ID is `fm.scoped_id(id_confirm)`.
   - When drawing `Button("Confirm")` in `on_render`, passing `.id(fm.id(id_confirm))` stamps that exact scoped `Id` into `Frame::m_hit_buffer` so left-clicking the button matches the modal target (`src/focus.cpp:136-146, 654`).
   - Note that `TextInput(confirm_input)` automatically tags `Id(&confirm_input)` and `FocusManager::handle_mouse` checks both `hit` and `scoped_hit = this->scoped_id(hit)` (`src/focus.cpp:640, 654`), so `TextInput`, `TextArea`, `List`, and `Tabs` work without an explicit `.id(...)` call.
4. **Closing the modal (`fm.pop_scope()`)**:
   - `pop_scope()` erases all targets with `modal_scope == Id("delete_modal")` from `m_targets` and restores focus to the saved background target (`src/focus.cpp:754-776`).

<!-- src: src/focus.cpp:89-146, 636-777 -->

## Common mistakes

:::caution
Do not create a local `auto modal = fm.modal_scope(Id("dialog"));` stack variable inside `on_render` for an interactive `App` dialog. `IdScope`'s destructor calls `fm.pop_scope()` as soon as `on_render` returns—which erases all modal targets and exits the modal scope before the user's next keystroke or mouse click is processed in `App::dispatch_event()`. Use `fm.push_scope(id, ScopeMode::Modal)` when opening the modal and `fm.pop_scope()` when closing it (or store an `std::optional<IdScope>` that lives across frames while the modal is open).
:::

- **Forgetting to dim or override background `.focused(...)` on manually styled buttons**: When a modal opens, `FocusManager` calls `set_focus(false)` on stateful targets (`ListState`, `TextInputState`, `TextAreaState`, `TabsState`). For a stateless `Button` whose focus is driven manually via `.focused(...)`, pass `.focused(!modal_open && fm.is_focused(id))` so it does not appear focused behind the modal.

## See also

- [FocusManager](/api/focus-manager/)
- [Focus Management Concept](/concepts/focus-management/)
- [Focus System Architecture](/architecture/focus-system/)
