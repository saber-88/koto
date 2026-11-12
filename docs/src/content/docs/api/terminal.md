---
title: Terminal
description: POSIX termios raw mode controller, TTY detector, escape mode toggle interface, and signal recovery handler
sidebar:
  label: Terminal
  order: 2
---

## Purpose and header

`koto::Terminal` manages POSIX `termios` raw mode transitions, alternate screen switching, cursor visibility, Kitty keyboard and SGR mouse escape sequences, `ColorMode` detection, and signal/crash restoration handlers.
Header: `koto/terminal.hpp` (included by `koto/koto.hpp`).

<!-- src: include/koto/terminal.hpp:1-95 -->

## Types and members

### `TerminalError` and `TerminalErrorInfo`

| Symbol | Kind | Description | Default |
| :--- | :--- | :--- | :--- |
| `TerminalError::None` | Enum value (`0`) | Operation succeeded. | `0` |
| `TerminalError::NotATty` | Enum value (`1`) | `STDIN_FILENO` or `STDOUT_FILENO` is not a TTY (`isatty() == 0`). | None |
| `TerminalError::GetAttrFailed` | Enum value (`2`) | `tcgetattr(STDIN_FILENO, ...)` returned non-zero. | None |
| `TerminalError::SetAttrFailed` | Enum value (`3`) | `tcsetattr(STDIN_FILENO, TCSANOW, ...)` returned non-zero. | None |
| `TerminalErrorInfo::code` | Field (`TerminalError`) | High-level error code. | `TerminalError::None` |
| `TerminalErrorInfo::system_errno` | Field (`int`) | System `errno` captured at failure (`ENOTTY` for `NotATty`). | `0` |
| `TerminalErrorInfo::message()` | `constexpr std::string_view message() const noexcept` | Static string view describing `code`. | `"Success"` |

<!-- src: include/koto/terminal.hpp:10-38 -->

### `Terminal` methods

| Member | Signature | Description | Default |
| :--- | :--- | :--- | :--- |
| `Terminal()` | `Terminal()` | Checks `isatty` on stdin and stdout, runs `detect_system_color_mode()`, and queries `ioctl(STDOUT_FILENO, TIOCGWINSZ)`. | `m_rows = 24`, `m_cols = 80` fallback |
| `~Terminal()` | `~Terminal()` | Calls `restore_terminal()`. | None |
| `is_tty()` | `bool is_tty() const noexcept` | Returns `true` if both `STDIN_FILENO` and `STDOUT_FILENO` are TTYs. | `false` when piped |
| `in_raw_mode()` | `bool in_raw_mode() const noexcept` | Returns `true` while raw mode is active. | `false` |
| `color_mode()` | `ColorMode color_mode() const noexcept` | Returns active `ColorMode`. | Auto-detected |
| `color_mode(mode)` | `Terminal& color_mode(ColorMode mode) noexcept` | Sets active `ColorMode` and returns `*this`. | None |
| `enter_raw_mode()` | `auto enter_raw_mode() -> std::expected<void, TerminalErrorInfo>` | Enters raw mode, installs signal handlers, enters alternate screen (`[?1049h`), and enables bracketed paste (`[?2004h`) and Kitty keyboard (`[>1u`). | None |
| `restore_terminal()` | `auto restore_terminal() -> void` | Restores `termios` (`TCSAFLUSH`), disables mouse/Kitty/paste/alt-screen, shows cursor, and uninstalls signal handlers. | None |
| `get_cols()` / `cols()` / `width()` | `int get_cols() const` / `int cols() const noexcept` / `int width() const noexcept` | Returns cached terminal column count (`m_cols`). | `80` fallback |
| `get_rows()` / `rows()` / `height()` | `int get_rows() const` / `int rows() const noexcept` / `int height() const noexcept` | Returns cached terminal row count (`m_rows`). | `24` fallback |
| `set_dimensions()` | `void set_dimensions(int r, int c) noexcept` | Updates cached `m_rows = r` and `m_cols = c`. | None |
| `show_cursor()` / `hide_cursor()` | `void show_cursor() const` / `void hide_cursor() const` | Emits `[?25h` or `[?25l` if `is_tty()`. | None |
| `enable_bracketed_paste()` / `disable_bracketed_paste()` | `void enable_bracketed_paste() const` / `void disable_bracketed_paste() const` | Emits `[?2004h` or `[?2004l` if `is_tty()`. | Enabled by `enter_raw_mode()` |
| `enable_kitty_keyboard()` / `disable_kitty_keyboard()` | `void enable_kitty_keyboard() const` / `void disable_kitty_keyboard() const` | Emits `[>1u` (push flag 1) or `[<u` (pop) if `is_tty()`. | Enabled by `enter_raw_mode()` |
| `enable_mouse_capture()` / `disable_mouse_capture()` | `void enable_mouse_capture() const` / `void disable_mouse_capture() const` | Emits `[?1000h[?1006h` or `[?1006l[?1003l[?1002l[?1000l` if `is_tty()`. | Off until requested |
| `enable_mouse_drag()` / `disable_mouse_drag()` | `void enable_mouse_drag() const` / `void disable_mouse_drag() const` | Emits `[?1002h` or `[?1002l` if `is_tty()`. | Off |
| `enable_mouse_hover()` / `disable_mouse_hover()` | `void enable_mouse_hover() const` / `void disable_mouse_hover() const` | Emits `[?1003h` or `[?1003l` if `is_tty()`. | Off |

<!-- src: include/koto/terminal.hpp:40-94 -->
<!-- src: src/terminal.cpp:214-446 -->

## Examples

```cpp
#include <koto/koto.hpp>
#include <cassert>

using namespace koto;

int main()
{
    Terminal term;
    term.color_mode(ColorMode::Ansi256);
    assert(term.color_mode() == ColorMode::Ansi256);
    assert(term.rows() > 0 && term.cols() > 0);

    // Safe no-op when running in a non-TTY test environment
    term.enable_mouse_hover();
    term.disable_mouse_hover();
    return 0;
}
```

## Errors and edge cases

- **`termios` flags cleared in raw mode**: `enter_raw_mode()` clears `ECHO | ICANON | ISIG | IEXTEN` on `c_lflag` and `IXON | ICRNL` on `c_iflag`, then applies `tcsetattr(STDIN_FILENO, TCSANOW, &raw)`. It also calls `std::setlocale(LC_ALL, "")`.
- **Termination vs. crash signals**: `enter_raw_mode()` installs handlers on a 64 KB alternate signal stack (`sigaltstack`). For termination signals (`SIGTERM`, `SIGINT`, `SIGQUIT`, `SIGHUP`), if the host application previously installed a custom handler, `Terminal` delegates to that handler without resetting raw mode; if `SIG_DFL` was active, it restores the terminal and re-raises the signal. If `SIGHUP` had `SIG_IGN` (such as under `nohup`), `Terminal` does not overwrite it.
- **Crash recovery**: For `SIGSEGV`, `SIGABRT`, `SIGBUS`, `SIGFPE`, `SIGILL`, `std::terminate`, and `std::atexit`, `restore_minimal()` immediately writes `[?1006l[?1003l[?1002l[?1000l[<u[?2004l[?25h[?1049l` and calls `tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_saved)` before chaining to any previous crash handler.

<!-- src: src/terminal.cpp:16-210 -->

## See also

- [App](/api/app/)
- [Frame](/api/frame/)
- [Cell and Color](/api/cell-color/)
- [Troubleshooting](/deep-dives/troubleshooting/)
