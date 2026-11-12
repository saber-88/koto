---
title: Events and Input Decoding
description: How EventLoop decodes raw terminal byte streams, disambiguates Escape and paste sequences, and batches input bursts
sidebar:
  label: Events and Input
  order: 4
---

## The idea in plain words

A terminal delivers keyboard presses, mouse clicks, and pasted text over a single byte stream (`STDIN_FILENO`), while window resizes arrive asynchronously as POSIX `SIGWINCH` signals. `koto::EventLoop` turns those raw bytes and signals into strongly typed `Event` values without busy-waiting or splitting multi-byte sequences.

## How koto implements it

1. **Dual-FD `poll()` with POSIX self-pipe**: `EventLoop` creates a non-blocking pipe `m_pipe_fds[2]` and registers `EventLoop::on_sigwinch` for `SIGWINCH` (`src/event.cpp:1043-1066`). `on_sigwinch` writes `'R'` to the pipe, and `EventLoop::wakeup()` writes `'W'`. `EventLoop::poll(timeout_ms)` blocks in `::poll(pfds, 2, eff_timeout)` watching both `STDIN_FILENO` and `m_pipe_fds[0]` (`src/event.cpp:896-936`).
2. **Kernel burst draining via `ioctl(FIONREAD)`**: When `STDIN_FILENO` is readable, `EventLoop::poll()` queries `ioctl(STDIN_FILENO, FIONREAD, &available)` and reads all available bytes into `m_pending_input` before parsing (`src/event.cpp:984-1013`). In `App::run()`, after `m_loop.poll(wait_ms)` returns the first event, a `while (m_running && m_loop.has_pending_event())` loop drains all complete events from `m_pending_input` with `m_loop.poll(0)` before running a single `draw_frame()` pass (`src/app.cpp:282-313`).
3. **35 ms escape deadline vs. 500 ms bracketed paste deadline**:
   - When `m_pending_input` starts with an incomplete short sequence (such as a lone ``, `[`, `O`, `]`, or an incomplete multi-byte UTF-8 lead byte), `EventLoop` sets `m_escape_deadline = now + 35ms`. If more bytes arrive, the deadline resets; if 35 ms elapse, `flush_incomplete_escape()` emits `Key::Escape` (or `Alt+'['`, `Alt+Shift+'O'`, `Alt+']'`, or drops an orphaned non-ESC byte) (`src/event.cpp:702-754`).
   - When `m_pending_input` starts with `[200~` (bracketed paste start without `[201~`), `EventLoop` sets `m_paste_deadline = now + 500ms` (resetting on each incoming chunk, up to a `4 MB` safety cap) so slow pastes over SSH or `tmux` are never torn apart by the 35 ms escape timer (`src/event.cpp:642-655, 671-700`).

<!-- src: src/event.cpp:357-1082 -->
<!-- src: src/app.cpp:226-314 -->

```cpp
EventLoop loop;
loop.feed("[1;");
// Incomplete CSI sequence: has_pending() is true, has_pending_event() is false
loop.feed("5C");
// Complete sequence: Ctrl+Right is ready
Event ev = loop.poll(0);
static_cast<void>(ev);
```

## Consequences for your code

- **Zero CPU usage when idle**: If you do not register `app.on_tick(...)`, `App::run()` passes `wait_ms = -1` to `m_loop.poll(-1)` whenever no bytes are pending (`src/app.cpp:237-269`), sleeping inside the kernel until input or a signal arrives.
- **Pasted text arrives atomically**: With bracketed paste enabled (automatic in `Terminal::enter_raw_mode()`), multi-line clipboard pastes arrive as a single `PasteEvent` with `
` and `` normalized to `
`.

## See also

- [Events and EventLoop](/api/events/)
- [Event Loop Architecture](/architecture/event-loop/)
- [Keyboard Guide](/guides/keyboard/)
