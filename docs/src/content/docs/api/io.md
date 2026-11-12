---
title: I/O, Widget Base, and Umbrella Header
description: Reference for write_all in koto/io.hpp, the abstract Widget base class in koto/widget.hpp, and koto/koto.hpp
sidebar:
  label: I/O and Widget Base
  order: 10
---

## Purpose and header

This page covers the remaining three public headers in `include/koto/`:
- `koto/io.hpp`: `EINTR`-safe, short-write-safe POSIX descriptor writer `koto::write_all`.
- `koto/widget.hpp`: Abstract base class `koto::Widget` implemented by all widgets.
- `koto/koto.hpp`: Umbrella header that includes all public headers except `koto/io.hpp`.

<!-- src: include/koto/io.hpp:1-30 -->
<!-- src: include/koto/widget.hpp:1-15 -->
<!-- src: include/koto/koto.hpp:1-25 -->

## Types and members

| Symbol | Header | Signature | Description |
| :--- | :--- | :--- | :--- |
| `write_all(fd, data)` | `koto/io.hpp:13` | `[[nodiscard]] inline std::expected<void, int> write_all(int fd, std::string_view data)` | Loops `::write(fd, ptr, rem)` until all bytes in `data` are written, retrying on `EINTR` and returning `std::unexpected(EIO)` on zero write or `std::unexpected(errno)` on error. |
| `Widget` | `koto/widget.hpp:8` | `class Widget` | Abstract base class for all drawable components. |
| `Widget::~Widget()` | `koto/widget.hpp:11` | `virtual ~Widget() = default` | Virtual destructor for polymorphic ownership (used by `Vbox` and `Hbox`). |
| `Widget::draw(frame, area)` | `koto/widget.hpp:12` | `virtual void draw(Frame& frame, Area area) = 0` | Pure virtual method that renders the widget into `frame` within `area`. |
| `koto/koto.hpp` | `koto/koto.hpp:1` | `#pragma once` umbrella | Includes `frame.hpp`, `cell.hpp`, `terminal.hpp`, `layout.hpp`, `widget.hpp`, `box.hpp`, `event.hpp`, `text.hpp`, `color.hpp`, `vbox.hpp`, `hbox.hpp`, `list.hpp`, `text_input.hpp`, `utf8.hpp`, `app.hpp`, `rule.hpp`, `gauge.hpp`, `sparkline.hpp`, `text_area.hpp`, `tabs.hpp`, `id.hpp`, `focus.hpp`, `fill.hpp`, and `button.hpp`. |

<!-- src: include/koto/io.hpp:13-29 -->
<!-- src: include/koto/widget.hpp:8-14 -->
<!-- src: include/koto/koto.hpp:1-25 -->

## Examples

### Implementing a custom `Widget`

```cpp
#include <koto/koto.hpp>
#include <cassert>

using namespace koto;

class StatusBanner : public Widget
{
public:
    explicit StatusBanner(std::string_view msg) : m_msg(msg) {}

    void draw(Frame& frame, Area area) override
    {
        if (!area.is_valid()) return;
        Fill(Color::Blue).draw(frame, area);
        Text(m_msg).fg(Color::White).align(TextAlign::Center).draw(frame, area);
    }

private:
    std::string_view m_msg;
};

int main()
{
    Frame frame(1, 20);
    StatusBanner("ONLINE").draw(frame, frame.area());
    assert(frame.to_plain_text_row(0).find("ONLINE") != std::string::npos);
    return 0;
}
```

## Errors and edge cases

- **`koto/io.hpp` is not included by `koto/koto.hpp`**: If you call `koto::write_all()` directly in application code, include `<koto/io.hpp>` explicitly.
- **Short writes and `EINTR`**: When flushing large ANSI diff buffers over a PTY or SSH pipe, `::write()` can write fewer bytes than requested or fail transiently with `EINTR`. `write_all()` loops until `rem == 0` so partial ANSI sequences are never left truncated on `STDOUT_FILENO`.

<!-- src: include/koto/io.hpp:13-29 -->

## See also

- [Frame](/api/frame/)
- [Terminal](/api/terminal/)
- [Vbox](/widgets/vbox/)
- [Hbox](/widgets/hbox/)
