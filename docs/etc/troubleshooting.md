# Troubleshooting & Terminal Compatibility Guide

Comprehensive diagnostic manual covering terminal emulator compatibility matrices, environment variable configuration, color quantization anomalies, raw mode crash recovery, and key protocol debugging.

---

## Terminal Emulator Support Matrix

Terminal emulators vary widely in their standards compliance, color depth, escape sequence handling, and keyboard protocols:

| Terminal Emulator | TrueColor (24-bit) | SGR Mouse (1006) | Kitty Keyboard Protocol | Bracketed Paste | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **Ghostty** | ✅ Full | ✅ Full | ✅ Full | ✅ Full | Modern, GPU-accelerated, highly standards-compliant. |
| **Kitty** | ✅ Full | ✅ Full | ✅ Full | ✅ Full | Reference implementation for `CSI u` and synchronized updates. |
| **WezTerm** | ✅ Full | ✅ Full | ✅ Full | ✅ Full | Full support across Linux, macOS, and Windows. |
| **Alacritty** | ✅ Full | ✅ Full | ❌ Partial | ✅ Full | Fast GPU terminal; lacks full Kitty protocol. |
| **iTerm2** (macOS) | ✅ Full | ✅ Full | ❌ Partial | ✅ Full | Requires CSI u feature toggle in preferences. |
| **macOS Terminal.app**| ❌ (256 only) | ⚠️ Basic | ❌ None | ✅ Full | Lacks TrueColor; `COLORTERM` is unset. |
| **Windows Terminal** | ✅ Full | ✅ Full | ❌ Partial | ✅ Full | ConPTY translation layer active. |
| **Linux Virtual Console** (`/dev/tty1`)| ❌ (16 colors) | ❌ None | ❌ None | ❌ None | Bare Linux framebuffer / VGA console. |

---

## Common Issues & Diagnoses

### 1. Colors Appear Washed Out, Quantized, or Missing

#### Symptom
Colors specified with RGB TrueColor (e.g., `Color::from_rgb(120, 200, 255)`) appear as coarse 16-color or 256-color approximations, or fail to render.

#### Root Cause
`libkoto` detects color support at startup by inspecting environment variables:
1. If `COLORTERM=truecolor` or `COLORTERM=24bit`, it enables 24-bit RGB TrueColor.
2. If `TERM` contains `256color`, it enables 256-color palette quantization.
3. Otherwise, it falls back to standard 16 ANSI colors or monochrome.

Certain SSH sessions, multiplexers (`tmux`, `screen`), or desktop terminal defaults fail to export `COLORTERM`.

#### Resolution
Ensure your shell profile exports `COLORTERM`:
```bash
# Add to ~/.bashrc or ~/.zshrc
export COLORTERM=truecolor
```

If using `tmux`, add the following to `~/.tmux.conf`:
```tmux
set -g default-terminal "tmux-256color"
set -as terminal-features ",xterm-256color:RGB"
```

---

### 2. Terminal Echo Broken or Terminal State Locked After Crash

#### Symptom
After terminating a program with `kill -9` (`SIGKILL`) or an unhandled segmentation fault, typing into the shell produces invisible characters or escape sequence noise (`^[[A`), and pressing Enter produces stair-stepped lines.

#### Root Cause
To read input character-by-character without waiting for the user to press Enter, `libkoto` configures the terminal in **Raw Mode** via `tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw)`:
* It disables canonical mode (`ICANON`).
* It disables character echo (`ECHO`).
* It enables mouse tracking and hides the cursor.

Normal termination via [`App::quit()`](../api/app.md), standard signals (`SIGINT`, `SIGTERM`, `SIGHUP`), or fatal crash signals (`SIGSEGV`, `SIGABRT`) invokes handlers that automatically restore the original cooked mode. However, uncatchable signals (`SIGKILL`) bypass cleanup.

#### Resolution
Run the POSIX terminal reset command:
```bash
stty sane
```
Or reset the terminal completely:
```bash
reset
```

If mouse tracking is still stuck on (causing the terminal to print coordinates when you click):
```bash
printf "\033[?1000l\033[?1002l\033[?1003l\033[?1006l\033[?25h"
```

---

### 3. Wide Unicode Characters or Emojis Overlap and Break Borders

#### Symptom
Box borders appear misaligned, or text following an emoji or CJK character is displaced by one column:

```
Expected:
┌──────────────┐
│ Status: 🚀 Ok│
└──────────────┘

Observed:
┌──────────────┐
│ Status: 🚀 Ok │
└──────────────┘
```

#### Root Cause
1. **Terminal Font Discrepancy**: Monospace fonts might lack wide glyphs, causing the terminal emulator to substitute fallback glyphs with non-standard column metrics.
2. **Outdated Terminal Emulator Unicode Tables**: `libkoto` uses the official **Unicode 16.0** standard for character widths. If the terminal emulator is running an older Unicode standard (e.g., Unicode 11.0 or 12.0), the emulator may treat certain newly-promoted emojis as width 1 while `libkoto` treats them as width 2 (or vice versa).

#### Resolution
* **Use Monospace Nerd Fonts**: Use modern programming fonts with dedicated emoji glyph coverage, such as *JetBrains Mono*, *Fira Code*, or *Hack*, paired with *Noto Color Emoji*.
* **Update Terminal Emulator**: Ensure your terminal emulator is up to date (e.g., Ghostty, WezTerm, or Kitty).

---

### 4. Key Combinations (`Shift+Tab`, `Ctrl+Enter`) Not Detected

#### Symptom
Pressing `Shift+Tab` triggers standard `Tab`, or pressing `Ctrl+Enter` is indistinguishable from standard `Enter`.

#### Root Cause
Legacy ANSI terminals do not have distinct byte escape sequences for modified keys. For example, both `Enter` and `Ctrl+Enter` transmit a single carriage return byte (`0x0D`).

#### Resolution
`libkoto` automatically enables the **Kitty Keyboard Protocol** on terminals that advertise support:
```
\x1b[>1u
```
When running in compliant terminals (Kitty, Ghostty, WezTerm, Foot), `libkoto` decodes:
* `Key::Tab` with `KeyModifiers::Shift`
* `Key::Enter` with `KeyModifiers::Ctrl`
* Key release events and modifier-only keypresses

If running on terminals without Kitty protocol support (such as macOS `Terminal.app`), applications should design alternative keybindings (e.g., using `Alt+j` / `Alt+k` or arrow keys).

---

## C++ Language Features Utilized

1. **RAII Cleanup Idiom (C++98/C++11)**: `Terminal::~Terminal()` restores cooked mode and reenables the cursor, preventing terminal corruption even when exceptions or early returns occur.
2. **POSIX Signal & Crash Handlers**: Global signal handlers catch `SIGINT`, `SIGTERM`, and `SIGHUP` (preserving `nohup`), while dedicated alternate stack handlers (`sigaltstack`) catch `SIGSEGV`/`SIGABRT` and `std::set_terminate` catches uncaught exceptions to invoke clean restoration sequences.
3. **`std::getenv` Integration**: Inspects environment variables without external dependencies.
4. **`std::expected` Monadic Error Handling (C++23)**: Returns descriptive error categories when terminal initialization fails, eliminating unhandled exceptions.
