---
title: Limitations and FAQ
description: Source-verified platform, protocol, threading, and widget limitations and answers to common questions
sidebar:
  label: Limitations and FAQ
  order: 98
---

## Platform and terminal limitations

- **POSIX-only implementation**: `libkoto` directly includes `<termios.h>`, `<poll.h>`, `<fcntl.h>`, `<sys/ioctl.h>`, `<unistd.h>`, and `<csignal>`. It does not compile against the native Windows Console API without a POSIX compatibility layer such as WSL2.
- **Process-global signal and `termios` recovery state**: `src/terminal.cpp:16-25` stores `g_saved`, `g_old_term`, `g_old_hup`, `g_old_int`, `g_old_quit`, `g_old_segv`, `g_old_abrt`, `g_old_bus`, `g_old_fpe`, `g_old_ill`, and `g_altstack_buf` at file scope, and `EventLoop::m_pipe_write_fd` (`src/event.cpp:16`) is a `static volatile sig_atomic_t`. Only one `Terminal` / `EventLoop` / `App` instance should be active in a process at a time.
- **Mouse protocol**: `EventLoop` parses SGR 1006 mouse sequences (`[<btn;col;rowM` and `[<btn;col;rowm`, `src/event.cpp:36-107`). Legacy X10 (`[M...`) mouse sequences are not parsed by `EventLoop`.
- **Bracketed paste size cap**: `EventLoop` caps buffered incomplete bracketed pastes (`[200~`) at `4 * 1024 * 1024` bytes (4 MB) or `500` ms of inactivity before flushing (`src/event.cpp:645, 778`).
- **Inline `Grapheme` capacity**: `Grapheme::data` is `char data[36]`, storing up to `35` UTF-8 bytes per cell (`include/koto/utf8.hpp:28, 90`). Clusters longer than 35 bytes are truncated at the last complete codepoint boundary that fits within 35 bytes.

<!-- src: src/terminal.cpp:14-26 -->
<!-- src: src/event.cpp:16, 36-107, 643-655 -->
<!-- src: include/koto/utf8.hpp:26-120 -->

## Widget and focus limitations

- **Single SGR style per `Text` widget**: `koto::Text` applies a single `m_fg`, `m_bg`, and `m_attrs` bitmask to all characters it draws (`include/koto/text.hpp:57-58`). To render multiple colors on the same line, split the line with `Layout::horizontal` or write cells directly to `Frame`.
- **No mouse-click cursor positioning inside `TextInput` or `TextArea`**: Left-clicking a `TextInput` or `TextArea` registered in `FocusManager` focuses the widget, but `TextInputState` has no `handle_mouse` method and `TextAreaState::handle_mouse` only handles `WheelUp` and `WheelDown` (`src/text_area.cpp:397-410`). To move the caret on click, handle `MouseEvent` in `app.on_mouse(...)` using `visual_col_to_byte_offset()`.
- **`Key::Tab` inside `FocusManager` vs. `TextArea`**: `FocusManager::handle_key` intercepts `Key::Tab` and `Key::BackTab` to cycle focus before invoking the active widget's `handle_key` (`src/focus.cpp:550-576`). To insert 4 spaces with `Tab` inside a focused `TextArea`, register a `fm.interceptor(...)` that forwards `Key::Tab` to `editor_state.handle_event(key)` when `fm.is_focused(editor_state)`.

<!-- src: include/koto/text.hpp:52-59 -->
<!-- src: src/text_area.cpp:397-410 -->
<!-- src: src/focus.cpp:550-576 -->

## Frequently asked questions

### Why does `.id("my_name")` fail to compile with an ambiguous overload error?

`koto::Id` defines both `constexpr Id(const char* str) noexcept` and `Id(const void* ptr) noexcept` (`include/koto/id.hpp:64, 75`). When passing a string literal to a method taking `Id` (such as `.id(...)`), both conversions are standard/user-defined pointer conversions of equal rank. Wrap the string literal in `Id("my_name")`:

```cpp
Box().id(Id("my_name")).draw(frame, box_area);
```

<!-- src: include/koto/id.hpp:63-76 -->

### How do I wake the UI from a background worker thread?

Mutate shared state using atomic variables or a mutex, then call `app.request_redraw()` (`include/koto/app.hpp:37`). `request_redraw()` sets `m_needs_draw = true` and calls `m_loop.wakeup()`, which writes 1 byte (`'W'`) to the non-blocking self-pipe to wake `EventLoop::poll()` immediately (`src/app.cpp:78-82`, `src/event.cpp:828-835`).

### Why does my application not quit when I press `Ctrl+C` or `q`?

`Terminal::enter_raw_mode()` disables `ISIG` (`src/terminal.cpp:246`), and `App` does not hardcode any exit keys. Register an `app.on_key` handler (or `fm.interceptor`) that calls `app.quit()`.

## See also

- [Troubleshooting](/deep-dives/troubleshooting/)
- [App](/api/app/)
- [FocusManager](/api/focus-manager/)
- [Id](/api/id/)
