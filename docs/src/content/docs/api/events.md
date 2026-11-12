---
title: Events and EventLoop
description: Keyboard, mouse, resize, paste, and tick event types and the POSIX EventLoop parser and poller
sidebar:
  label: Events
  order: 5
---

## Purpose and header

`koto/event.hpp` defines the input and timing event structs (`KeyEvent`, `MouseEvent`, `ResizeEvent`, `PasteEvent`, `TickEvent`), the `Event` variant, key and mouse enums (`Key`, `MouseButton`, `MouseAction`), and `EventLoop`.
Header: `koto/event.hpp` (included by `koto/koto.hpp`).

<!-- src: include/koto/event.hpp:1-205 -->

## Types and members

### `Key` and `KeyEvent`

| Symbol | Kind | Description | Default |
| :--- | :--- | :--- | :--- |
| `Key::None` | Enum value (`0`) | Unset key token. | `0` |
| `Key::Up`, `Down`, `Left`, `Right` | Enum values (`0x80000000`..`0x80000003`) | Directional arrow keys. | None |
| `Key::Home`, `End`, `PageUp`, `PageDown` | Enum values | Navigation keys. | None |
| `Key::Enter`, `Backspace`, `Delete`, `Insert`, `Tab`, `BackTab` | Enum values | Editing and focus-cycling keys. | None |
| `Key::Escape` | Enum value | Escape key. | None |
| `Key::F1` .. `Key::F12` | Enum values | Function keys F1 through F12. | None |
| `Key::ShiftUp` .. `Key::ShiftRight` | Enum values | Shift + arrow keys. | None |
| `Key::CtrlUp` .. `Key::CtrlRight` | Enum values | Ctrl + arrow keys. | None |
| `Key::AltUp` .. `Key::AltRight` | Enum values | Alt + arrow keys. | None |
| `KeyEvent::ch` | Field (`uint32_t`) | Unicode codepoint or `static_cast<uint32_t>(Key)`. | `0` |
| `KeyEvent::ctrl`, `shift`, `alt`, `meta` | Fields (`bool`) | Modifier flags decoded from legacy control bytes, CSI modifiers, or Kitty CSI-u. | `false` |
| `KeyEvent::is(Key k) const` | Method | Matches `k`, matching both composite enums (e.g. `Key::CtrlRight`) and base key + modifier flag (`ch == Key::Right && ctrl`). For plain `Key::Up`/`Down`/`Left`/`Right`, requires `!shift && !ctrl && !alt`. | None |
| `KeyEvent::is_char() const` | Method | Returns `true` when `(ch >= 32 && ch < 127) \|\| (ch >= 160 && ch < 0x80000000)`. | None |

<!-- src: include/koto/event.hpp:14-74 -->

### `MouseButton`, `MouseAction`, `MouseEvent`, `ResizeEvent`, `PasteEvent`, `TickEvent`, and `Event`

| Symbol | Kind | Description | Default |
| :--- | :--- | :--- | :--- |
| `MouseButton` | Enum class (`uint8_t`) | `Left = 0`, `Middle = 1`, `Right = 2`, `WheelUp = 64`, `WheelDown = 65`, `WheelLeft = 66`, `WheelRight = 67`, `None = 255`. | None |
| `MouseAction` | Enum class (`uint8_t`) | `Press`, `Release`, `Move`. | None |
| `MouseEvent::row`, `col` | Fields (`int`) | 0-based terminal coordinates (converted from 1-based SGR 1006 input). | `0` |
| `MouseEvent::button` | Field (`MouseButton`) | Active button or wheel direction. | `MouseButton::Left` |
| `MouseEvent::action` | Field (`MouseAction`) | Press, release, or motion state. | `MouseAction::Press` |
| `MouseEvent::shift`, `alt`, `ctrl` | Fields (`bool`) | Active keyboard modifiers during mouse action. | `false` |
| `MouseEvent::target` | Field (`Id`) | Hit-tested `Id` populated by `App::dispatch_event()` from `Frame::hit_at(row, col)`. | `Id::none()` |
| `MouseEvent::is_press()` / `is_release()` / `is_move()` | Methods (`constexpr`) | Checks `action`. | None |
| `MouseEvent::is_left_press()` / `is_right_press()` | Methods (`constexpr`) | Checks `is_press()` and `button`. | None |
| `MouseEvent::is_wheel_up()` / `down()` / `left()` / `right()` | Methods (`constexpr`) | Checks wheel button values. | None |
| `ResizeEvent` | Struct | Contains `uint32_t rows` and `uint32_t cols`. | None |
| `TickEvent` | Struct | Empty struct indicating timeout or wakeup without input. | None |
| `PasteEvent` | Struct | Contains `std::string text` from bracketed paste (`[200~`..`[201~`). | `""` |
| `Event` | Struct (`std::variant`) | `std::variant<KeyEvent, ResizeEvent, TickEvent, PasteEvent, MouseEvent>` with `is_key()`, `is_resize()`, `is_tick()`, `is_paste()`, `is_mouse()`, pointer getters `as_*()`, and reference getters `key()`, `resize()`, `paste()`, `mouse()`. | None |

<!-- src: include/koto/event.hpp:76-150 -->

### `EventLoop`

| Member | Signature | Description | Default |
| :--- | :--- | :--- | :--- |
| `EventLoop()` / `~EventLoop()` | Constructor / Destructor | Creates non-blocking `O_CLOEXEC` self-pipe and installs `SIGWINCH` handler; destructor closes pipe FDs and restores previous `SIGWINCH` disposition. | Non-copyable |
| `poll(ms)` | `Event poll(std::int32_t ms = 16)` | Returns next buffered event or waits via `::poll()` up to `ms` milliseconds (`-1` waits indefinitely). | `ms = 16` |
| `has_pending()` | `bool has_pending() const noexcept` | Returns `!m_pending_input.empty()`. | `false` |
| `has_pending_event()` | `bool has_pending_event() const noexcept` | Returns `true` if `m_pending_input` contains a complete event or an expired deadline (35 ms escape / 500 ms paste / 4 MB cap). | `false` |
| `is_eof()` | `bool is_eof() const noexcept` | Returns `true` if `read(STDIN_FILENO, ...)` returned `0`, `EIO`, or `EBADF`. | `false` |
| `parse(bytes)` | `static Event parse(std::string_view bytes)` | Parses a single complete sequence from `bytes` without mutating an `EventLoop` instance. | None |
| `on_sigwinch(int)` | `static void on_sigwinch(int)` | Writes `'R'` to the static self-pipe write descriptor while preserving `errno`. | None |
| `feed(bytes)` | `void feed(std::string_view bytes)` | Appends `bytes` to `m_pending_input` and updates paste/escape deadlines. | None |
| `wakeup()` | `void wakeup() noexcept` | Writes `'W'` to the self-pipe write descriptor to wake `poll()` from any thread. | None |

<!-- src: include/koto/event.hpp:153-204 -->
<!-- src: src/event.cpp:614-1082 -->

## Examples

```cpp
#include <koto/koto.hpp>
#include <cassert>

using namespace koto;

int main()
{
    Event key_ev = EventLoop::parse("[13;5u"); // Kitty CSI-u Ctrl+Enter
    assert(key_ev.is_key());
    assert(key_ev.key().is(Key::Enter));
    assert(key_ev.key().ctrl);

    Event mouse_ev = EventLoop::parse("[<0;10;5M"); // SGR left press at col 10, row 5 (1-based)
    assert(mouse_ev.is_mouse());
    assert(mouse_ev.mouse().is_left_press());
    assert(mouse_ev.mouse().col == 9 && mouse_ev.mouse().row == 4);
    return 0;
}
```

## Errors and edge cases

- **Ctrl+letter normalization**: Both legacy control bytes (`0x01`..`0x1A`) and Kitty CSI-u `Ctrl + 'a'..'z'` normalize `KeyEvent::ch` to uppercase `'A'..'Z'` with `ctrl = true` (`src/event.cpp:303-306`, `src/event.cpp:571-576`).
- **Space key representation**: There is no `Key::Space` enum value. The spacebar produces a character `KeyEvent` with `key.ch == ' '` (`32`) where `key.is_char()` is `true`. `Ctrl+Space` (`0x00`) produces `KeyEvent{' ', true, false, false, false}`.
- **Kitty key release suppression**: In Kitty keyboard protocol (`CSI u`), events with sub-parameter `:3` (key release) and standalone lock/modifier codepoints (`57358..57363`, `57441..57454`) are ignored and return `TickEvent` (`src/event.cpp:191-194`, `src/event.cpp:297-300`).
- **Incomplete paste and escape deadlines**: Incomplete escape sequences wait 35 ms before flushing as `Key::Escape` (or `Alt+'['`, `Alt+Shift+'O'`, `Alt+']'`). Incomplete bracketed pastes (`[200~`) wait up to 500 ms of inactivity or a 4 MB buffer cap before flushing as a `PasteEvent`.

<!-- src: src/event.cpp:269-313 -->
<!-- src: src/event.cpp:629-754 -->

## See also

- [App](/api/app/)
- [FocusManager](/api/focus-manager/)
- [Events and Input Concept](/concepts/events-input/)
- [Event Loop Architecture](/architecture/event-loop/)
