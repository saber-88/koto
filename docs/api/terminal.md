# `koto::Terminal`

Low-level POSIX terminal driver interface managing raw mode state transitions, TTY detection, Kitty keyboard protocol negotiation, SGR mouse tracking, bracketed paste mode, and color capability quantization.

---

## Header & Class Synopsis

```cpp
#include <koto/terminal.hpp>
```

```cpp
namespace koto
{
    enum class ColorMode
    {
        NoColor,
        Ansi16,
        Ansi256,
        TrueColor
    };

    struct TerminalErrorInfo
    {
        std::string_view message() const noexcept;
        int system_errno = 0;
    };

    class Terminal
    {
    public:
        Terminal();
        ~Terminal();

        // Raw Mode & Lifecycle
        [[nodiscard]] auto enter_raw_mode() -> std::expected<void, TerminalErrorInfo>;
        [[nodiscard]] auto restore_terminal() -> std::expected<void, TerminalErrorInfo>;
        [[nodiscard]] bool in_raw_mode() const noexcept;
        [[nodiscard]] bool is_tty() const noexcept;

        // Protocols & Feature Flags
        void enable_kitty_keyboard();
        void disable_kitty_keyboard();
        void enable_mouse_tracking();
        void disable_mouse_tracking();
        void enable_bracketed_paste();
        void disable_bracketed_paste();

        // Hardware Cursor Control
        void hide_cursor();
        void show_cursor();

        // Color Capabilities
        [[nodiscard]] ColorMode color_mode() const noexcept;
        void set_color_mode(ColorMode mode) noexcept;

        // Dimensions
        [[nodiscard]] auto size() const -> std::expected<std::pair<int, int>, TerminalErrorInfo>;
    };
}
```

---

## Member Functions Reference

| Method | Signature | Description |
|:---|:---|:---|
| **`enter_raw_mode()`** | `auto enter_raw_mode() -> std::expected<void, TerminalErrorInfo>` | Saves original termios, disables `ECHO`/`ICANON`, and switches to alternate screen buffer. |
| **`restore_terminal()`**| `auto restore_terminal() -> std::expected<void, TerminalErrorInfo>` | Restores original cooked mode, reenables cursor, and leaves alternate screen. |
| **`in_raw_mode()`** | `bool in_raw_mode() const noexcept` | Returns `true` if raw mode is currently active. |
| **`is_tty()`** | `bool is_tty() const noexcept` | Queries whether standard output is connected to an interactive TTY (`isatty()`). |
| **`enable_kitty_keyboard()`** | `void enable_kitty_keyboard()` | Emits `\033[>1u` to negotiate Kitty keyboard protocol flag 1. |
| **`enable_mouse_tracking()`** | `void enable_mouse_tracking()` | Emits SGR 1006 escape sequences for button, motion, and wheel tracking. |
| **`enable_bracketed_paste()`**| `void enable_bracketed_paste()` | Emits `\033[?2004h` to capture multi-line clipboard streams inside `PasteEvent`. |
| **`hide_cursor()`** | `void hide_cursor()` | Emits `\033[?25l` to hide the hardware cursor during rendering passes. |
| **`show_cursor()`** | `void show_cursor()` | Emits `\033[?25h` to restore hardware cursor visibility. |
| **`color_mode()`** | `ColorMode color_mode() const noexcept` | Returns the detected or configured color quantization depth. |
| **`size()`** | `auto size() const -> std::expected<std::pair<int, int>, TerminalErrorInfo>` | Queries window rows and columns via `ioctl(STDOUT_FILENO, TIOCGWINSZ)`. |

---

## Usage Examples

### 1. RAII Terminal Lifecycle Management

```cpp
#include <koto/koto.hpp>

using namespace koto;

void run_scoped_terminal()
{
    Terminal term;

    auto res = term.enter_raw_mode();
    if (!res) {
        std::fprintf(stderr, "Failed to enter raw mode: %.*s\n",
                     static_cast<int>(res.error().message().size()),
                     res.error().message().data());
        return;
    }

    term.enable_mouse_tracking();
    term.enable_bracketed_paste();
    term.hide_cursor();

    // Terminal settings are automatically restored when `term` leaves scope!
}
```

---

## Invariants & Safety Guarantees

1. **Destructor Teardown Invariant**: `~Terminal()` guarantees that the terminal is cleanly returned to cooked mode, restoring normal character echo and leaving alternate screen buffers.
2. **Signal Handler Protection**: Low-level signal handlers (`SIGINT`, `SIGTERM`, `SIGHUP`, `SIGQUIT`) invoke async-signal-safe `restore_minimal()` to prevent corrupted terminal states upon abnormal termination. `SIGHUP` respects inherited `SIG_IGN` to preserve `nohup` operation.
3. **Crash Signal Recovery (`sigaltstack`)**: Fatal signals (`SIGSEGV`, `SIGABRT`, `SIGBUS`, `SIGFPE`, `SIGILL`) run on a dedicated 64 KB alternate signal stack with `SA_ONSTACK | SA_RESETHAND`, restoring canonical terminal mode before core dumping.
4. **Unhandled Exception Protection**: A registered `std::set_terminate` hook calls `restore_minimal()` on uncaught C++ exceptions.
5. **Monadic Error Reporting**: All POSIX system call failures preserve the exact `errno` value inside `TerminalErrorInfo::system_errno`.

---

## C++ Language Features Utilized

1. **`std::expected<void, TerminalErrorInfo>` (C++23)**: Monadic error propagation avoiding runtime exception overhead.
2. **RAII Idiom (C++98/C++11)**: Deterministic terminal hardware state cleanup.
3. **Scoped Strongly-Typed Enums (`enum class`) (C++11)**: `ColorMode` isolates color configuration depths.
4. **`[[nodiscard]]` (C++17)**: Guarantees raw mode activation results are checked.
5. **`std::string_view` (C++17)**: Zero-copy error message views.
