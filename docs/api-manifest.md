# libkoto API Manifest, Widget Input Bindings, Glossary, and Page Plan

- **Repository**: `https://github.com/saber-88/koto.git`
- **Verified Commit (`git rev-parse HEAD`)**: `51c6c1dc7d25cb805ca620ef9b8af699941e14f1`
- **Target Standard**: C++23 (`CMAKE_CXX_STANDARD 23`)

## Glossary of Canonical Terms

Every documentation page uses these exact terms without synonym substitution:

| Term | Canonical Definition |
| :--- | :--- |
| **widget** | A concrete class deriving from `koto::Widget` (`include/koto/widget.hpp:8`) that implements `void draw(Frame& frame, Area area) override` to render into a `frame` within a target `area`. |
| **state struct** | A mutable struct (`ListState`, `TabsState`, `TextInputState`, `TextAreaState`, or a user type satisfying `FocusableState`) that stores persistent widget state across frames and processes input events via `handle_event(const KeyEvent&)` and optional `handle_mouse(const MouseEvent&)` or `insert(const std::string&)`. |
| **frame** | An instance of `koto::Frame` (`include/koto/frame.hpp:14`) managing the front and back `cell` buffers, the spatial `hit buffer`, harvested tag bounding boxes, cursor state, and batched ANSI output stream. |
| **area** | A `koto::Area` struct (`include/koto/layout.hpp:10`) defining a 0-based rectangular terminal region `{row, col, width, height}`. |
| **cell** | A 64-byte `koto::Cell` struct (`include/koto/cell.hpp:29`) holding an inline `Grapheme` cluster (`char data[36]`), primary codepoint `ch`, 24-bit `fg` and `bg` colors, column `width`, `continuation` flag, and `TextAttr` bitmask `attrs`. |
| **hit buffer** | The per-cell integer buffer inside `koto::Frame` (`m_hit_buffer`, initialized to `TAG_NONE = -1`) that maps `(row, col)` screen coordinates to `Id` tokens in `O(1)` via `Frame::hit_at(row, col)`. |
| **id** | A 32-bit `koto::Id` token (`include/koto/id.hpp:52`) hashed via `fnv1a_32` from strings, pointers, enums, integers, or `(name, index)` / `(name, row, col)` tuples, with `TAG_NONE` (`-1`) representing `Id::none()`. |
| **focus ring** | The ordered vector of `FocusTarget` entries (`m_targets`) inside `koto::FocusManager` (`include/koto/focus.hpp:139`) cycled in 1D order by `Tab` (`next()`) and `Shift+Tab` / `Key::BackTab` (`prev()`). |
| **scope** | An `Id` namespace pushed onto `FocusManager` via `FocusManager::scope(Id)` (`ScopeMode::Normal`) that combines parent and child `Id` tokens via `Id::combine` (`scoped_id`) to prevent collisions across component instances. |
| **modal scope** | A focus-trapping scope pushed onto `FocusManager` via `FocusManager::modal_scope(Id)` or `push_scope(id, ScopeMode::Modal)` that saves active focus, restricts keyboard navigation and mouse events to targets registered inside that modal, shields background widgets from clicks, and prunes modal targets while restoring prior focus on exit. |
| **constraint** | A `koto::Constraint` value (`include/koto/layout.hpp:95`) constructed via `Constraint::fixed(n)`, `Constraint::percent(n)`, or `Constraint::fill()` to partition an `area` along a 1D axis or 2D `Grid`. |

## Public Symbol Inventory (`include/koto/*.hpp`)

All private `m_*` fields and anonymous-namespace helpers are omitted per specification.

### `include/koto/koto.hpp`

| Symbol | Kind | Header | Line | One-line purpose | Default values |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `koto.hpp` | Umbrella header | `include/koto/koto.hpp` | 1–25 | Includes all public `koto` headers except `io.hpp`. | N/A |

### `include/koto/app.hpp`

| Symbol | Kind | Header | Line | One-line purpose | Default values |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `koto::App` | Class | `include/koto/app.hpp` | 12 | Top-level application runtime owning `Terminal`, `Frame`, and `EventLoop`. | Non-copyable (`delete`) |
| `App::App()` | Constructor | `include/koto/app.hpp` | 15 | Initializes `Terminal`, `Frame(m_term)`, and `EventLoop` without entering raw mode. | `m_running = true`, `m_needs_draw = true` |
| `App::~App()` | Destructor | `include/koto/app.hpp` | 16 | Defaulted destructor; member `Terminal` restores terminal state on teardown. | `= default` |
| `App::on_key(handler)` | Method | `include/koto/app.hpp` | 21 | Registers callback `std::function<void(const KeyEvent&)>` for unconsumed key events. | `nullptr` |
| `App::on_paste(handler)` | Method | `include/koto/app.hpp` | 22 | Registers callback `std::function<void(const std::string&)>` for bracketed paste text. | `nullptr` |
| `App::on_mouse(handler)` | Method | `include/koto/app.hpp` | 23 | Registers callback `std::function<void(const MouseEvent&)>` and calls `m_term.enable_mouse_capture()`. | `nullptr` |
| `App::on_tick(std::function<void()>)` | Method | `include/koto/app.hpp` | 24 | Registers parameterless ~60 FPS (16 ms) periodic animation tick callback. | `nullptr` |
| `App::on_tick(std::function<void(double)>)` | Method | `include/koto/app.hpp` | 25 | Registers ~60 FPS (16 ms) periodic tick callback receiving elapsed seconds `dt`. | `nullptr` |
| `App::on_render(handler)` | Method | `include/koto/app.hpp` | 26 | Registers frame drawing callback `std::function<void(Frame&)>`. | `nullptr` |
| `App::use_focus(FocusManager& fm)` | Method | `include/koto/app.hpp` | 34 | Binds `FocusManager`, enables SGR mouse capture, syncs geometry after render, and routes input. | `nullptr` |
| `App::quit()` | Method | `include/koto/app.hpp` | 36 | Sets `m_running = false` and calls `m_loop.wakeup()` to exit `run()`. | N/A |
| `App::request_redraw() noexcept` | Method | `include/koto/app.hpp` | 37 | Sets `m_needs_draw = true` and calls `m_loop.wakeup()` from any thread. | N/A |
| `App::run()` | Method | `include/koto/app.hpp` | 38 | Enters raw mode, hides cursor, runs the event/render loop, and returns `std::expected<void, TerminalErrorInfo>`. | N/A |
| `App::run(render_function)` | Method | `include/koto/app.hpp` | 39 | Sets `on_render(render_function)` and calls `run()`. | N/A |
| `App::area() const noexcept` | Method | `include/koto/app.hpp` | 42 | Returns `m_frame.area()` (`Area{0, 0, cols, rows}`). | N/A |
| `App::hit_at(int row, int col) const` | Method | `include/koto/app.hpp` | 43 | Returns the `Id` in `m_frame.hit_at(row, col)`. | N/A |
| `App::hit_at(const MouseEvent& mouse) const` | Method | `include/koto/app.hpp` | 44 | Returns `m_frame.hit_at(mouse.row, mouse.col)`. | N/A |
| `App::draw_frame(bool flush = true)` | Method | `include/koto/app.hpp` | 47 | Clears back buffer, runs `on_render`, syncs `FocusManager` geometry, and calls `m_frame.render(flush)`. | `flush = true` |
| `App::dispatch_event(const Event& ev)` | Method | `include/koto/app.hpp` | 48 | Dispatches a `ResizeEvent`, `PasteEvent`, `MouseEvent`, or `KeyEvent` to `FocusManager` and app handlers. | N/A |
| `App::frame() noexcept` | Method | `include/koto/app.hpp` | 49 | Returns mutable reference to internal `Frame`. | N/A |
| `App::frame() const noexcept` | Method | `include/koto/app.hpp` | 50 | Returns const reference to internal `Frame`. | N/A |

### `include/koto/terminal.hpp`

| Symbol | Kind | Header | Line | One-line purpose | Default values |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `koto::TerminalError` | Enum class (`uint8_t`) | `include/koto/terminal.hpp` | 10 | Error codes for terminal raw mode initialization (`None = 0`, `NotATty`, `GetAttrFailed`, `SetAttrFailed`). | `None = 0` |
| `koto::TerminalErrorInfo` | Struct | `include/koto/terminal.hpp` | 18 | Pairs `TerminalError code` with POSIX `int system_errno` and `message()`. | `code = TerminalError::None`, `system_errno = 0` |
| `TerminalErrorInfo::code` | Field (`TerminalError`) | `include/koto/terminal.hpp` | 20 | High-level terminal error enum value. | `TerminalError::None` |
| `TerminalErrorInfo::system_errno` | Field (`int`) | `include/koto/terminal.hpp` | 21 | Captured `errno` (`ENOTTY` or `tcgetattr`/`tcsetattr` `errno`). | `0` |
| `TerminalErrorInfo::message() const noexcept` | Method (`constexpr`) | `include/koto/terminal.hpp` | 23 | Returns human-readable `std::string_view` description of `code`. | N/A |
| `koto::Terminal` | Class | `include/koto/terminal.hpp` | 40 | POSIX terminal controller managing `termios` raw mode, alternate screen, signals, and escape modes. | Non-copyable, non-movable |
| `Terminal::Terminal()` | Constructor | `include/koto/terminal.hpp` | 43 | Checks `isatty(STDIN_FILENO)` and `isatty(STDOUT_FILENO)`, detects `ColorMode`, and queries `ioctl(TIOCGWINSZ)`. | `m_rows = 24`, `m_cols = 80` fallback |
| `Terminal::~Terminal()` | Destructor | `include/koto/terminal.hpp` | 44 | Calls `restore_terminal()` if in raw mode. | N/A |
| `Terminal::is_tty() const noexcept` | Method | `include/koto/terminal.hpp` | 52 | Returns `true` if both stdin and stdout are TTYs. | `false` in pipes/CI |
| `Terminal::in_raw_mode() const noexcept` | Method | `include/koto/terminal.hpp` | 53 | Returns `true` if `enter_raw_mode()` succeeded and has not been restored. | `false` |
| `Terminal::color_mode() const noexcept` | Method | `include/koto/terminal.hpp` | 54 | Returns active `ColorMode`. | `detect_system_color_mode()` |
| `Terminal::color_mode(ColorMode mode) noexcept` | Method | `include/koto/terminal.hpp` | 55 | Overrides active `ColorMode` and returns `Terminal&`. | N/A |
| `Terminal::enter_raw_mode()` | Method | `include/koto/terminal.hpp` | 58 | Saves `termios`, installs signal/crash handlers, disables `ECHO\|ICANON\|ISIG\|IEXTEN` and `IXON\|ICRNL`, enters alt screen (`?1049h`), enables bracketed paste (`?2004h`) and Kitty keyboard (`>1u`). | Returns `std::expected<void, TerminalErrorInfo>` |
| `Terminal::restore_terminal()` | Method | `include/koto/terminal.hpp` | 59 | Restores `termios` (`TCSAFLUSH`), disables mouse/Kitty/paste/alt-screen, shows cursor, and uninstalls signal handlers. | N/A |
| `Terminal::get_cols() const` | Method | `include/koto/terminal.hpp` | 61 | Returns cached column count (`m_cols`). | `80` fallback |
| `Terminal::get_rows() const` | Method | `include/koto/terminal.hpp` | 62 | Returns cached row count (`m_rows`). | `24` fallback |
| `Terminal::cols() const noexcept` | Method | `include/koto/terminal.hpp` | 63 | Returns cached column count (`m_cols`). | `80` fallback |
| `Terminal::rows() const noexcept` | Method | `include/koto/terminal.hpp` | 64 | Returns cached row count (`m_rows`). | `24` fallback |
| `Terminal::width() const noexcept` | Method | `include/koto/terminal.hpp` | 65 | Alias for `cols()`. | `80` fallback |
| `Terminal::height() const noexcept` | Method | `include/koto/terminal.hpp` | 66 | Alias for `rows()`. | `24` fallback |
| `Terminal::set_dimensions(int r, int c) noexcept` | Method | `include/koto/terminal.hpp` | 67 | Updates cached `m_rows = r` and `m_cols = c`. | N/A |
| `Terminal::show_cursor() const` | Method | `include/koto/terminal.hpp` | 69 | Writes `[?25h` to `STDOUT_FILENO` when `is_tty()`. | N/A |
| `Terminal::hide_cursor() const` | Method | `include/koto/terminal.hpp` | 70 | Writes `[?25l` to `STDOUT_FILENO` when `is_tty()`. | N/A |
| `Terminal::enable_bracketed_paste() const` | Method | `include/koto/terminal.hpp` | 72 | Writes `[?2004h` when `is_tty()`. | N/A |
| `Terminal::disable_bracketed_paste() const` | Method | `include/koto/terminal.hpp` | 73 | Writes `[?2004l` when `is_tty()`. | N/A |
| `Terminal::enable_kitty_keyboard() const` | Method | `include/koto/terminal.hpp` | 74 | Writes `[>1u` when `is_tty()`. | N/A |
| `Terminal::disable_kitty_keyboard() const` | Method | `include/koto/terminal.hpp` | 75 | Writes `[<u` when `is_tty()`. | N/A |
| `Terminal::enable_mouse_capture() const` | Method | `include/koto/terminal.hpp` | 76 | Writes `[?1000h[?1006h` when `is_tty()`. | N/A |
| `Terminal::disable_mouse_capture() const` | Method | `include/koto/terminal.hpp` | 77 | Writes `[?1006l[?1003l[?1002l[?1000l` when `is_tty()`. | N/A |
| `Terminal::enable_mouse_drag() const` | Method | `include/koto/terminal.hpp` | 78 | Writes `[?1002h` when `is_tty()`. | N/A |
| `Terminal::disable_mouse_drag() const` | Method | `include/koto/terminal.hpp` | 79 | Writes `[?1002l` when `is_tty()`. | N/A |
| `Terminal::enable_mouse_hover() const` | Method | `include/koto/terminal.hpp` | 80 | Writes `[?1003h` when `is_tty()`. | N/A |
| `Terminal::disable_mouse_hover() const` | Method | `include/koto/terminal.hpp` | 81 | Writes `[?1003l` when `is_tty()`. | N/A |

### `include/koto/frame.hpp`

| Symbol | Kind | Header | Line | One-line purpose | Default values |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `koto::Frame` | Class | `include/koto/frame.hpp` | 14 | Move-only double-buffered grid of `Cell`s, `hit buffer`, and tag bounds. | Copy deleted, move defaulted |
| `Frame::Frame(Terminal& t)` | Constructor | `include/koto/frame.hpp` | 19 | Constructs a frame attached to `Terminal& t` with dimensions `t.get_rows()` by `t.get_cols()`. | Front buffer `Cell{U'ÿ'}`, hit buffer `TAG_NONE` |
| `Frame::Frame(int rows, int cols)` | Constructor | `include/koto/frame.hpp` | 20 | Constructs a headless frame with `rows` and `cols` (negative clamped to `0`) and `m_term = nullptr`. | Front buffer `Cell{U'ÿ'}`, hit buffer `TAG_NONE` |
| `Frame::handle_resize(int rows, int cols)` | Method | `include/koto/frame.hpp` | 28 | Resizes buffers to `rows * cols`, fills front buffer with `U'ÿ'` to force repaint, and clears tags. | N/A |
| `Frame::set_cell(int row, int col, Cell c, Id tag_id = Id::none())` | Method | `include/koto/frame.hpp` | 29 | Writes `c` to back buffer with wide-char continuation repair, margin clamping, and optional `tag_id` stamping. | `tag_id = Id::none()` |
| `Frame::get_cell(int row, int col) const` | Method | `include/koto/frame.hpp` | 30 | Returns a copy of the back-buffer `Cell` at `(row, col)`, or default `Cell{}` if out of bounds. | N/A |
| `Frame::operator[](int row, int col)` | Method | `include/koto/frame.hpp` | 31 | C++23 2D subscript returning mutable `Cell&` in back buffer (or thread-local dummy if out of bounds). | N/A |
| `Frame::operator[](int row, int col) const` | Method | `include/koto/frame.hpp` | 32 | C++23 2D subscript returning `const Cell&` in back buffer (or static empty `Cell` if out of bounds). | N/A |
| `Frame::fill(Area area, Cell c, Id tag_id = Id::none())` | Method | `include/koto/frame.hpp` | 33 | Calls `set_cell` across every coordinate in `area` and records `area` in `m_tag_bounds` if `tag_id` is set. | `tag_id = Id::none()` |
| `Frame::fill(Area area, char32_t ch, uint32_t fg = COLOR_DEFAULT, uint32_t bg = COLOR_DEFAULT, Id tag_id = Id::none())` | Method | `include/koto/frame.hpp` | 34 | Fills `area` with `Cell{ch, fg, bg, 1, false}` and optional `tag_id`. | `fg = COLOR_DEFAULT`, `bg = COLOR_DEFAULT`, `tag_id = Id::none()` |
| `Frame::clear_back_buffer()` | Method | `include/koto/frame.hpp` | 35 | Resets back buffer to default `Cell{}`, hit buffer to `TAG_NONE`, clears `m_tag_bounds`, and hides cursor. | N/A |
| `Frame::render(bool flush = true)` | Method | `include/koto/frame.hpp` | 36 | Diffs back buffer against front buffer inside DEC Mode 2026 (`[?2026h`..`l`) and writes to stdout if `flush` and `m_term->is_tty()`. | `flush = true` |
| `Frame::set_cursor(int row, int col)` | Method | `include/koto/frame.hpp` | 37 | Marks hardware cursor visible and sets its position to `(row, col)` for this frame. | N/A |
| `Frame::area() const` | Method | `include/koto/frame.hpp` | 38 | Returns `Area{0, 0, m_cols, m_rows}`. | N/A |
| `Frame::rows() const noexcept` | Method | `include/koto/frame.hpp` | 39 | Returns `m_rows`. | N/A |
| `Frame::cols() const noexcept` | Method | `include/koto/frame.hpp` | 40 | Returns `m_cols`. | N/A |
| `Frame::height() const noexcept` | Method | `include/koto/frame.hpp` | 41 | Alias for `rows()`. | N/A |
| `Frame::width() const noexcept` | Method | `include/koto/frame.hpp` | 42 | Alias for `cols()`. | N/A |
| `Frame::tag_cell(int row, int col, Id tag_id)` | Method | `include/koto/frame.hpp` | 45 | Stamps `tag_id` into `m_hit_buffer` at `(row, col)` and unions `Area{row, col, 1, 1}` into `m_tag_bounds`. | N/A |
| `Frame::tag_area(Area area, Id tag_id)` | Method | `include/koto/frame.hpp` | 46 | Stamps `tag_id` into `m_hit_buffer` across `area` (clipped to frame) and unions `area` into `m_tag_bounds`. | N/A |
| `Frame::hit_at(int row, int col) const` | Method | `include/koto/frame.hpp` | 47 | Returns the `Id` at `(row, col)` in `m_hit_buffer`, or `Id::none()` if out of bounds. | N/A |
| `Frame::clear_hit_buffer(Id default_tag = Id::none())` | Method | `include/koto/frame.hpp` | 48 | Fills `m_hit_buffer` with `default_tag.value()` and clears `m_tag_bounds`. | `default_tag = Id::none()` |
| `Frame::bounds_of(Id tag_id) const` | Method | `include/koto/frame.hpp` | 49 | Returns `std::optional<Area>` containing the union bounding box of `tag_id` if present. | `std::nullopt` if absent |
| `Frame::all_tag_bounds() const` | Method | `include/koto/frame.hpp` | 50 | Returns `const std::unordered_map<uint32_t, Area>&` of all harvested tag bounds. | N/A |
| `Frame::to_plain_text() const` | Method | `include/koto/frame.hpp` | 53 | Serializes all rows via `to_plain_text_row(r)` joined by `
`. | N/A |
| `Frame::to_plain_text_row(int r) const` | Method | `include/koto/frame.hpp` | 54 | Serializes row `r` to UTF-8 (skipping continuation cells and trimming trailing spaces). | `""` if out of bounds |
| `Frame::raw_output() const noexcept` | Method | `include/koto/frame.hpp` | 55 | Returns `std::string_view` of the ANSI byte stream generated by the last `render()` call. | N/A |
| `Frame::cursor_row() const` | Method | `include/koto/frame.hpp` | 56 | Returns current cursor row (`-1` if unset). | `-1` |
| `Frame::cursor_col() const` | Method | `include/koto/frame.hpp` | 57 | Returns current cursor column (`-1` if unset). | `-1` |
| `Frame::cursor_visible() const` | Method | `include/koto/frame.hpp` | 58 | Returns `true` if `set_cursor` was called after `clear_back_buffer()`. | `false` |

### `include/koto/cell.hpp`

| Symbol | Kind | Header | Line | One-line purpose | Default values |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `koto::TextAttr` | Enum class (`uint8_t`) | `include/koto/cell.hpp` | 9 | Bitmask flags for SGR text attributes (`None = 0`, `Bold = 1<<0`, `Dim = 1<<1`, `Italic = 1<<2`, `Underline = 1<<3`, `Reverse = 1<<4`, `Strike = 1<<5`). | `None = 0` |
| `operator\|(TextAttr a, TextAttr b) noexcept` | Function (`constexpr`) | `include/koto/cell.hpp` | 20 | Bitwise OR returning `uint8_t` attribute mask. | N/A |
| `operator\|(uint8_t a, TextAttr b) noexcept` | Function (`constexpr`) | `include/koto/cell.hpp` | 24 | Bitwise OR combining `uint8_t` mask with `TextAttr`. | N/A |
| `koto::Cell` | Struct | `include/koto/cell.hpp` | 29 | 64-byte terminal grid cell (`static_assert(sizeof(Cell) == 64)` at line 74). | `ch = U' '`, `fg = COLOR_DEFAULT`, `bg = COLOR_DEFAULT`, `width = 1`, `continuation = false`, `attrs = 0` |
| `Cell::grapheme` | Field (`Grapheme`) | `include/koto/cell.hpp` | 30 | Inline UTF-8 grapheme cluster (`44` bytes). | `Grapheme{}` (`" "`) |
| `Cell::ch` | Field (`char32_t`) | `include/koto/cell.hpp` | 31 | Base Unicode codepoint. | `U' '` |
| `Cell::fg` | Field (`uint32_t`) | `include/koto/cell.hpp` | 32 | Foreground 24-bit RGB color or `COLOR_DEFAULT`. | `COLOR_DEFAULT` |
| `Cell::bg` | Field (`uint32_t`) | `include/koto/cell.hpp` | 33 | Background 24-bit RGB color or `COLOR_DEFAULT`. | `COLOR_DEFAULT` |
| `Cell::width` | Field (`uint8_t`) | `include/koto/cell.hpp` | 34 | Visual column width (`0`, `1`, or `2`). | `1` |
| `Cell::continuation` | Field (`bool`) | `include/koto/cell.hpp` | 35 | `true` if this cell is the right-hand companion of a width-2 cell. | `false` |
| `Cell::attrs` | Field (`uint8_t`) | `include/koto/cell.hpp` | 36 | Bitmask of `TextAttr` values. | `0` |
| `Cell::reserved[5]` | Field (`uint8_t[5]`) | `include/koto/cell.hpp` | 37 | Padding bytes aligning `sizeof(Cell)` to exactly 64 bytes. | `{}` |
| `Cell::Cell()` | Constructor (`constexpr`) | `include/koto/cell.hpp` | 39 | Default-constructs a space cell with default colors. | `= default` |
| `Cell::Cell(char32_t cp, uint32_t f, uint32_t b, int w, bool cont, uint8_t att)` | Constructor (`constexpr`) | `include/koto/cell.hpp` | 41 | Constructs a cell from a codepoint `cp`. | `f = COLOR_DEFAULT`, `b = COLOR_DEFAULT`, `w = 0`, `cont = false`, `att = 0` |
| `Cell::Cell(Grapheme g, uint32_t f, uint32_t b, bool cont, uint8_t att)` | Constructor (`constexpr`) | `include/koto/cell.hpp` | 47 | Constructs a cell from a `Grapheme` using `g.width`. | `f = COLOR_DEFAULT`, `b = COLOR_DEFAULT`, `cont = false`, `att = 0` |
| `Cell::Cell(Grapheme g, uint32_t f, uint32_t b, int w, bool cont, uint8_t att)` | Constructor (`constexpr`) | `include/koto/cell.hpp` | 52 | Constructs a cell from a `Grapheme` with explicit width `w`. | `cont = false`, `att = 0` |
| `Cell::view() const noexcept` | Method (`constexpr`) | `include/koto/cell.hpp` | 59 | Returns `grapheme.view()`. | N/A |
| `Cell::make_continuation() noexcept` | Static method (`constexpr`) | `include/koto/cell.hpp` | 64 | Returns a `Cell` with `continuation = true`. | N/A |
| `Cell::operator==(const Cell&) const` | Operator | `include/koto/cell.hpp` | 71 | Defaulted member-wise equality comparison. | `= default` |

### `include/koto/color.hpp`

| Symbol | Kind | Header | Line | One-line purpose | Default values |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `koto::COLOR_DEFAULT` | Constant (`uint32_t`) | `include/koto/color.hpp` | 10 | Sentinel value `0xFF000000` representing terminal default color (`SGR 39`/`49`). | `0xFF000000` |
| `koto::ColorMode` | Enum class (`uint8_t`) | `include/koto/color.hpp` | 12 | Terminal color output depth (`NoColor = 0`, `Ansi16 = 1`, `Ansi256 = 2`, `TrueColor = 3`). | N/A |
| `koto::rgb_to_ansi256(r, g, b) noexcept` | Function | `include/koto/color.hpp` | 20 | Quantizes 24-bit RGB to xterm 256-color index (`16..231` cube or `232..255` grayscale). | N/A |
| `koto::rgb_to_ansi16(r, g, b) noexcept` | Function | `include/koto/color.hpp` | 21 | Quantizes 24-bit RGB to nearest ANSI 16-color index (`0..15`) by Euclidean RGB distance. | N/A |
| `koto::detect_system_color_mode() noexcept` | Function | `include/koto/color.hpp` | 22 | Detects `ColorMode` from `NO_COLOR`, `TERM`, and `COLORTERM` environment variables. | `ColorMode::TrueColor` fallback |
| `koto::ColorParseError` | Enum class | `include/koto/color.hpp` | 24 | Parse error reason (`EmptyString`, `InvalidLength`, `InvalidHexDigit`). | N/A |
| `koto::try_parse_color(color)` | Function | `include/koto/color.hpp` | 31 | Parses named color or `#RGB`/`#RRGGBB`/`#RRGGBBAA` hex into `std::expected<uint32_t, ColorParseError>`. | N/A |
| `koto::parse_color(color)` | Function | `include/koto/color.hpp` | 32 | Calls `try_parse_color(color).value_or(COLOR_DEFAULT)`. | `COLOR_DEFAULT` on error |
| `koto::Color` | Struct | `include/koto/color.hpp` | 34 | 32-bit packed RGB value wrapper with implicit `uint32_t` conversion and named constants. | `value = COLOR_DEFAULT` |
| `Color::value` | Field (`uint32_t`) | `include/koto/color.hpp` | 36 | Packed `0x00RRGGBB` value or `COLOR_DEFAULT`. | `COLOR_DEFAULT` |
| `Color::Color() noexcept` | Constructor (`constexpr`) | `include/koto/color.hpp` | 38 | Constructs `Color` with `COLOR_DEFAULT`. | `= default` |
| `Color::Color(uint32_t val) noexcept` | Constructor (`constexpr`) | `include/koto/color.hpp` | 39 | Implicit constructor from raw `uint32_t`. | N/A |
| `Color::Color(std::string_view name_or_hex)` | Constructor | `include/koto/color.hpp` | 40 | Implicit constructor parsing color name or hex string via `parse_color`. | N/A |
| `Color::Color(const char* name_or_hex)` | Constructor | `include/koto/color.hpp` | 41 | Implicit constructor from C-string (`nullptr` yields `COLOR_DEFAULT`). | N/A |
| `Color::operator uint32_t() const noexcept` | Conversion (`constexpr`) | `include/koto/color.hpp` | 43 | Implicitly converts `Color` to `uint32_t`. | N/A |
| `Color::operator==(const Color&) const noexcept` | Operator (`constexpr`) | `include/koto/color.hpp` | 44 | Defaulted equality comparison. | `= default` |
| `Color::is_default() const noexcept` | Method (`constexpr`) | `include/koto/color.hpp` | 45 | Returns `value == COLOR_DEFAULT`. | N/A |
| `Color::Default` / `Reset` | Constant (`uint32_t`) | `include/koto/color.hpp` | 47–48 | `COLOR_DEFAULT` (`0xFF000000`). | `0xFF000000` |
| `Color::Black` .. `Color::Amber` | Constants (`uint32_t`) | `include/koto/color.hpp` | 49–63 | `Black` (`0x00000000`), `White` (`0x00FFFFFF`), `Red` (`0x00FF0000`), `Green` (`0x0000FF00`), `Blue` (`0x000000FF`), `Yellow` (`0x00FFFF00`), `Cyan` (`0x0000FFFF`), `Magenta` (`0x00FF00FF`), `Gray` (`0x00808080`), `DarkGray` (`0x00404040`), `LightGray` (`0x00C0C0C0`), `Orange` (`0x00FFA500`), `Emerald` (`0x0050C878`), `Teal` (`0x00008080`), `Amber` (`0x00FFBF00`). | See hex values |
| `Color::rgb(r, g, b) noexcept` | Static method (`constexpr`) | `include/koto/color.hpp` | 65 | Packs `(r << 16) \| (g << 8) \| b` into `uint32_t`. | N/A |
| `Color::Rgb(r, g, b) noexcept` | Static method (`constexpr`) | `include/koto/color.hpp` | 72 | Alias for `Color::rgb(r, g, b)`. | N/A |
| `Color::try_from_hex(hex)` | Static method | `include/koto/color.hpp` | 77 | Calls `try_parse_color(hex)`. | N/A |
| `Color::from_hex(hex)` | Static method | `include/koto/color.hpp` | 78 | Calls `parse_color(hex)`. | N/A |
| `Color::lerp(uint32_t c1, uint32_t c2, float t) noexcept` | Static method | `include/koto/color.hpp` | 79 | Linearly interpolates RGB channels between `c1` and `c2` for `t` in `[0.0f, 1.0f]`. | N/A |
| `Color::lerp(std::string_view hex1, std::string_view hex2, float t)` | Static method | `include/koto/color.hpp` | 80 | Parses `hex1` and `hex2` and calls `Color::lerp`. | N/A |

### `include/koto/event.hpp`

| Symbol | Kind | Header | Line | One-line purpose | Default values |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `koto::Key` | Enum class (`uint32_t`) | `include/koto/event.hpp` | 14 | Non-character key identifiers (`None = 0`, `Up = 0x80000000`, `Down`, `Left`, `Right`, `Home`, `End`, `PageUp`, `PageDown`, `Enter`, `Backspace`, `Delete`, `Insert`, `Tab`, `BackTab`, `Escape`, `F1`..`F12`, `ShiftUp`..`ShiftRight`, `CtrlUp`..`CtrlRight`, `AltUp`..`AltRight`). | `None = 0` |
| `koto::KeyEvent` | Struct | `include/koto/event.hpp` | 42 | Represents a decoded keyboard press with codepoint/key token and modifier flags. | `ch = 0`, `ctrl = false`, `shift = false`, `alt = false`, `meta = false` |
| `KeyEvent::is(Key k) const` | Method | `include/koto/event.hpp` | 50 | Matches `k`, normalizing modified arrow/tab keys and requiring clean modifiers (`!shift && !ctrl && !alt`) for plain `Up`/`Down`/`Left`/`Right`. | N/A |
| `KeyEvent::is_char() const` | Method | `include/koto/event.hpp` | 73 | Returns `true` when `(ch >= 32 && ch < 127) \|\| (ch >= 160 && ch < 0x80000000)`. | N/A |
| `koto::ResizeEvent` | Struct | `include/koto/event.hpp` | 76 | Terminal window resize notification with `uint32_t rows` and `uint32_t cols`. | N/A |
| `koto::TickEvent` | Struct | `include/koto/event.hpp` | 82 | Empty struct emitted when `EventLoop::poll` times out or wakes without input/resize. | N/A |
| `koto::PasteEvent` | Struct | `include/koto/event.hpp` | 85 | Bracketed paste payload with `std::string text` (`
` and `
` normalized to `
`). | `text = ""` |
| `koto::MouseButton` | Enum class (`uint8_t`) | `include/koto/event.hpp` | 90 | Mouse button or wheel identifier (`Left = 0`, `Middle = 1`, `Right = 2`, `WheelUp = 64`, `WheelDown = 65`, `WheelLeft = 66`, `WheelRight = 67`, `None = 255`). | N/A |
| `koto::MouseAction` | Enum class (`uint8_t`) | `include/koto/event.hpp` | 102 | Mouse action type (`Press`, `Release`, `Move`). | N/A |
| `koto::MouseEvent` | Struct | `include/koto/event.hpp` | 109 | Decoded SGR mouse event with 0-based `row`, `col`, `button`, `action`, modifiers, and hit-tested `target`. | `row = 0`, `col = 0`, `button = MouseButton::Left`, `action = MouseAction::Press`, `target = Id::none()` |
| `MouseEvent::is_press() const noexcept` | Method (`constexpr`) | `include/koto/event.hpp` | 120 | Returns `action == MouseAction::Press`. | N/A |
| `MouseEvent::is_release() const noexcept` | Method (`constexpr`) | `include/koto/event.hpp` | 121 | Returns `action == MouseAction::Release`. | N/A |
| `MouseEvent::is_move() const noexcept` | Method (`constexpr`) | `include/koto/event.hpp` | 122 | Returns `action == MouseAction::Move`. | N/A |
| `MouseEvent::is_left_press() const noexcept` | Method (`constexpr`) | `include/koto/event.hpp` | 123 | Returns `is_press() && button == MouseButton::Left`. | N/A |
| `MouseEvent::is_right_press() const noexcept` | Method (`constexpr`) | `include/koto/event.hpp` | 124 | Returns `is_press() && button == MouseButton::Right`. | N/A |
| `MouseEvent::is_wheel_up() const noexcept` | Method (`constexpr`) | `include/koto/event.hpp` | 125 | Returns `button == MouseButton::WheelUp`. | N/A |
| `MouseEvent::is_wheel_down() const noexcept` | Method (`constexpr`) | `include/koto/event.hpp` | 126 | Returns `button == MouseButton::WheelDown`. | N/A |
| `MouseEvent::is_wheel_left() const noexcept` | Method (`constexpr`) | `include/koto/event.hpp` | 127 | Returns `button == MouseButton::WheelLeft`. | N/A |
| `MouseEvent::is_wheel_right() const noexcept` | Method (`constexpr`) | `include/koto/event.hpp` | 128 | Returns `button == MouseButton::WheelRight`. | N/A |
| `koto::Event` | Struct (`std::variant`) | `include/koto/event.hpp` | 131 | `std::variant<KeyEvent, ResizeEvent, TickEvent, PasteEvent, MouseEvent>` with `is_*()`, `as_*()`, and `key()`/`resize()`/`paste()`/`mouse()` accessors. | N/A |
| `koto::EventLoop` | Class | `include/koto/event.hpp` | 153 | POSIX `poll()` demultiplexer, self-pipe `SIGWINCH`/wakeup handler, and escape/paste parser. | Non-copyable |
| `EventLoop::poll(std::int32_t ms = 16)` | Method | `include/koto/event.hpp` | 163 | Polls pending buffer, stdin, and self-pipe for up to `ms` milliseconds and returns next `Event`. | `ms = 16` |
| `EventLoop::has_pending() const noexcept` | Method | `include/koto/event.hpp` | 164 | Returns `!m_pending_input.empty()`. | N/A |
| `EventLoop::has_pending_event() const noexcept` | Method | `include/koto/event.hpp` | 165 | Returns `true` if a complete event or expired timeout (35 ms escape / 500 ms paste / 4 MB cap) is ready in memory. | N/A |
| `EventLoop::is_eof() const noexcept` | Method | `include/koto/event.hpp` | 166 | Returns `true` if stdin reached EOF or `EIO`/`EBADF`. | `false` |
| `EventLoop::parse(std::string_view bytes)` | Static method | `include/koto/event.hpp` | 169 | Statelessly parses a complete byte sequence into an `Event` (or `TickEvent` if incomplete). | N/A |
| `EventLoop::on_sigwinch(int)` | Static method | `include/koto/event.hpp` | 170 | Async-signal-safe `SIGWINCH` handler writing `'R'` to the self-pipe. | N/A |
| `EventLoop::feed(std::string_view bytes)` | Method | `include/koto/event.hpp` | 173 | Appends `bytes` into `m_pending_input` and refreshes paste/escape deadlines. | N/A |
| `EventLoop::wakeup() noexcept` | Method | `include/koto/event.hpp` | 187 | Writes `'W'` to the self-pipe to unblock `poll()` from any thread. | N/A |

### `include/koto/focus.hpp`

| Symbol | Kind | Header | Line | One-line purpose | Default values |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `koto::FocusableState` | Concept | `include/koto/focus.hpp` | 28 | Requires `s.focused` convertible to `bool` and `s.handle_event(k)` convertible to `bool`. | N/A |
| `koto::HasMouseHandler` | Concept | `include/koto/focus.hpp` | 34 | Requires `s.handle_mouse(m)` convertible to `bool`. | N/A |
| `koto::HasPasteHandler` | Concept | `include/koto/focus.hpp` | 39 | Requires `s.insert(str)` for `const std::string&`. | N/A |
| `koto::Direction` | Enum class (`uint8_t`) | `include/koto/focus.hpp` | 46 | 2D spatial movement direction (`Up`, `Down`, `Left`, `Right`). | N/A |
| `koto::ScopeMode` | Enum class (`uint8_t`) | `include/koto/focus.hpp` | 54 | Scope behavior mode (`Normal`, `Modal`). | N/A |
| `koto::FocusNavPolicy` | Enum class (`uint8_t`) | `include/koto/focus.hpp` | 60 | Keyboard focus policy (`TabOnly`, `SpatialArrows`, `AltArrows`, `Custom`). | `TabOnly` |
| `koto::FocusKeyInterceptor` | Type alias | `include/koto/focus.hpp` | 68 | `std::function<bool(FocusManager& fm, const KeyEvent& key)>`. | N/A |
| `koto::FocusChangeValidator` | Type alias | `include/koto/focus.hpp` | 69 | `std::function<bool(FocusManager& fm, Id from_id, Id to_id)>`. | N/A |
| `koto::FocusChangeObserver` | Type alias | `include/koto/focus.hpp` | 70 | `std::function<void(Id from_id, Id to_id)>`. | N/A |
| `koto::IdScope` | Class | `include/koto/focus.hpp` | 79 | Move-only RAII guard calling `fm.push_scope(scope_id, mode)` on construction and `fm.pop_scope()` on destruction. | `mode = ScopeMode::Normal` |
| `koto::FocusTarget` | Struct | `include/koto/focus.hpp` | 102 | Type-erased focus ring entry holding `id`, `bounds`, `modal_scope`, `enabled`, `visible`, and callbacks. | `bounds = {0,0,0,0}`, `enabled = true`, `visible = true` |
| `FocusTarget::is_focusable(bool require_geometry = true) const noexcept` | Method | `include/koto/focus.hpp` | 118 | Returns `enabled && visible` and (if `require_geometry`) `bounds.is_valid()`. | `require_geometry = true` |
| `koto::FocusManager` | Class | `include/koto/focus.hpp` | 139 | Central 1D focus ring, 2D spatial navigator, mouse router, and modal focus trap manager. | `m_policy = FocusNavPolicy::TabOnly` |
| `FocusManager::add(T& state, Id custom_id = Id::none())` | Template method | `include/koto/focus.hpp` | 147 | Registers a `FocusableState` (auto-binding `handle_mouse` and `insert` if satisfied) and returns `FocusManager&`. | `custom_id = Id::none()` (uses `Id(&state)`) |
| `FocusManager::add(Id id, on_key, on_mouse)` | Method | `include/koto/focus.hpp` | 190 | Registers a stateless target by `Id` with optional key/mouse callbacks (left click synthesizes `Key::Enter` if `on_mouse` is null and `on_key` is set). | `on_key = nullptr`, `on_mouse = nullptr` |
| `FocusManager::register_target(FocusTarget target)` | Method | `include/koto/focus.hpp` | 194 | Inserts or updates a `FocusTarget` in place and activates it if first target or entering active modal scope. | N/A |
| `FocusManager::remove(Id id)` / `remove(const T& state)` | Method | `include/koto/focus.hpp` | 197–201 | Unregisters target from `m_targets` and adjusts `m_active_index`. | N/A |
| `FocusManager::set_visible(Id id, bool visible)` / `is_visible(Id id) const` | Methods | `include/koto/focus.hpp` | 207, 209 | Sets or queries `visible` flag on target `id`. | `true` |
| `FocusManager::set_enabled(Id id, bool enabled)` / `is_enabled(Id id) const` | Methods | `include/koto/focus.hpp` | 208, 210 | Sets or queries `enabled` flag on target `id`. | `true` |
| `FocusManager::has_geometry() const noexcept` | Method | `include/koto/focus.hpp` | 211 | Returns `true` if any target has valid non-zero `bounds`. | N/A |
| `FocusManager::next()` / `prev()` | Methods | `include/koto/focus.hpp` | 214–215 | Advances focus forward (`Tab`) or backward (`BackTab`) across focusable targets in the active modal scope. | N/A |
| `FocusManager::focus(size_t index)` / `focus_id(Id id)` / `focus(Id id)` / `focus(const T& state)` | Methods | `include/koto/focus.hpp` | 216–225 | Shifts focus to target if allowed by modal scope and `can_focus` validator. | N/A |
| `FocusManager::sync_geometry(const Frame& frame)` | Method | `include/koto/focus.hpp` | 228 | Harvests `frame.bounds_of(target.id)` into each target and advances focus if active target became hidden/off-screen. | N/A |
| `FocusManager::move(Direction dir)` / `move_up()` / `move_down()` / `move_left()` / `move_right()` | Methods | `include/koto/focus.hpp` | 229–233 | Moves focus in 2D using `score = primary_dist + 2 * secondary_dist`. | N/A |
| `FocusManager::is_focused(Id id) const` / `is_focused(const T& state) const` | Methods | `include/koto/focus.hpp` | 236–240 | Returns `true` if `id` or `state` is the active focus target. | N/A |
| `FocusManager::current_index() const noexcept` / `current_id() const noexcept` | Methods | `include/koto/focus.hpp` | 245–246 | Returns active target index (`0` if empty) or `Id` (`Id::none()` if empty). | N/A |
| `FocusManager::size() const noexcept` / `empty() const noexcept` / `clear()` | Methods | `include/koto/focus.hpp` | 247–249 | Queries target count or clears all targets, scopes, and saved focus stack. | N/A |
| `FocusManager::handle_key(const KeyEvent& key)` | Method | `include/koto/focus.hpp` | 256 | Runs `interceptor`, `Tab`/`BackTab`, `AltArrows`, focused target `handle_key`, and `SpatialArrows`. | N/A |
| `FocusManager::handle_paste(const std::string& text)` | Method | `include/koto/focus.hpp` | 257 | Forwards `text` to focused target's `handle_paste` if present. | N/A |
| `FocusManager::navigation_policy(...)` / `enable_spatial_arrows(bool enable = true)` | Methods | `include/koto/focus.hpp` | 260–262 | Gets/sets `FocusNavPolicy` (`enable_spatial_arrows(true)` sets `SpatialArrows`, `false` sets `TabOnly`). | `FocusNavPolicy::TabOnly` |
| `FocusManager::interceptor(fn)` / `can_focus(fn)` / `on_focus_change(fn)` | Methods | `include/koto/focus.hpp` | 265–267 | Registers key pre-interceptor, focus transition validator, or focus change observer. | `nullptr` |
| `FocusManager::handle_mouse(const MouseEvent& mouse, const Frame& frame)` | Method | `include/koto/focus.hpp` | 273 | Routes mouse wheel/clicks via `frame.hit_at` and shields background targets when a modal scope is active. | N/A |
| `FocusManager::handle_mouse_click(int row, int col, const Frame& frame)` | Method | `include/koto/focus.hpp` | 278 | Synthesizes a left press at `(row, col)` and calls `handle_mouse`. | N/A |
| `FocusManager::scope(Id)` / `modal_scope(Id)` / `push_scope(Id, ScopeMode)` / `pop_scope()` | Methods | `include/koto/focus.hpp` | 281–284 | Manages normal and modal scopes (modal `pop_scope` erases modal targets and restores saved focus). | `mode = ScopeMode::Normal` |
| `FocusManager::scoped_id(Id)` / `id(Id)` / `id(const T& state)` / `active_modal_scope()` | Methods | `include/koto/focus.hpp` | 285–295 | Computes scope-combined `Id` or returns innermost active modal scope `Id`. | N/A |

### `include/koto/id.hpp`

| Symbol | Kind | Header | Line | One-line purpose | Default values |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `koto::TAG_NONE` | Constant (`int`) | `include/koto/id.hpp` | 10 | Sentinel value `-1` representing an untagged cell or empty `Id`. | `-1` |
| `koto::fnv1a_32(std::string_view str, uint32_t seed = 0x811c9dc5u) noexcept` | Function (`constexpr`) | `include/koto/id.hpp` | 19 | Computes 32-bit FNV-1a hash of `str`, remapping `0xFFFFFFFFu` to `0xFFFFFFFEu`. | `seed = 0x811c9dc5u` |
| `koto::fnv1a_32(uint64_t val, uint32_t seed = 0x811c9dc5u) noexcept` | Function (`constexpr`) | `include/koto/id.hpp` | 30 | Computes 32-bit FNV-1a hash of 8-byte integer `val`, remapping `0xFFFFFFFFu` to `0xFFFFFFFEu`. | `seed = 0x811c9dc5u` |
| `koto::Id` | Class | `include/koto/id.hpp` | 52 | 32-bit universal identifier token for widgets, state structs, and hit-buffer cells. | `m_value = static_cast<uint32_t>(TAG_NONE)` |
| `Id::Id()` | Constructor (`constexpr`) | `include/koto/id.hpp` | 56 | Constructs empty `Id` (`TAG_NONE`). | `TAG_NONE` |
| `Id::Id(uint32_t)` / `Id::Id(int)` | Constructors (`constexpr`) | `include/koto/id.hpp` | 59–60 | Constructs `Id` from raw numeric value. | N/A |
| `Id::Id(std::string_view)` / `Id::Id(const char*)` | Constructors (`constexpr`) | `include/koto/id.hpp` | 63–64 | Constructs `Id` by hashing string with `fnv1a_32`. | N/A |
| `Id::Id(std::string_view str, size_t index)` | Constructor (`constexpr`) | `include/koto/id.hpp` | 67 | Constructs indexed `Id` by chaining `fnv1a_32(index, fnv1a_32(str))`. | N/A |
| `Id::Id(std::string_view str, size_t row, size_t col)` | Constructor (`constexpr`) | `include/koto/id.hpp` | 71 | Constructs 2D matrix `Id` by chaining `fnv1a_32(col, fnv1a_32(row, fnv1a_32(str)))`. | N/A |
| `Id::Id(const void* ptr) noexcept` | Constructor | `include/koto/id.hpp` | 75 | Constructs `Id` by hashing 64-bit pointer address `reinterpret_cast<uintptr_t>(ptr)`. | N/A |
| `Id::Id(Enum e) noexcept` | Template constructor (`constexpr`) | `include/koto/id.hpp` | 82 | Constructs `Id` from any enum type (`std::is_enum_v<Enum>`). | N/A |
| `Id::combine(Id other) const noexcept` | Method (`constexpr`) | `include/koto/id.hpp` | 86 | Combines `this` seed with `other.m_value` via `fnv1a_32`. | N/A |
| `Id::value() const noexcept` | Method (`constexpr`) | `include/koto/id.hpp` | 92 | Returns underlying `uint32_t`. | N/A |
| `Id::is_none() const noexcept` | Method (`constexpr`) | `include/koto/id.hpp` | 93 | Returns `m_value == static_cast<uint32_t>(TAG_NONE)`. | N/A |
| `Id::operator bool() const noexcept` | Explicit conversion (`constexpr`) | `include/koto/id.hpp` | 94 | Returns `!is_none()`. | N/A |
| `Id::operator int() const noexcept` | Explicit conversion (`constexpr`) | `include/koto/id.hpp` | 95 | Returns `static_cast<int>(m_value)`. | N/A |
| `Id::operator==` | Operators (`constexpr`) | `include/koto/id.hpp` | 99–101 | Compares `Id` against `Id`, `int`, or `uint32_t`. | N/A |
| `Id::none() noexcept` | Static method (`constexpr`) | `include/koto/id.hpp` | 104 | Returns `Id()`. | `TAG_NONE` |

### `include/koto/layout.hpp`

| Symbol | Kind | Header | Line | One-line purpose | Default values |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `koto::Area` | Struct | `include/koto/layout.hpp` | 10 | Rectangular grid region `{row, col, width, height}`. | `row = 0, col = 0, width = 0, height = 0` |
| `Area::right() const` / `bottom() const` | Methods | `include/koto/layout.hpp` | 20–21 | Returns `col + width` and `row + height`. | N/A |
| `Area::inset(int top, int left, int bottom, int right) const` | Method | `include/koto/layout.hpp` | 23 | Returns inner `Area` shrunk by margins (width/height clamped to `>= 0`). | N/A |
| `Area::is_valid() const` | Method | `include/koto/layout.hpp` | 36 | Returns `width > 0 && height > 0`. | N/A |
| `Area::contains(int r, int c) const` | Method | `include/koto/layout.hpp` | 41 | Returns `r >= row && r < bottom() && c >= col && c < right()`. | N/A |
| `Area::center_row() const` / `center_col() const` | Methods | `include/koto/layout.hpp` | 46, 51 | Returns `row + height / 2` and `col + width / 2`. | N/A |
| `Area::union_with(Area other) const` | Method | `include/koto/layout.hpp` | 56 | Returns bounding `Area` enclosing both valid areas. | N/A |
| `Area::set_row(int)` / `set_col(int)` / `set_height(int)` / `set_width(int)` | Methods | `include/koto/layout.hpp` | 69–92 | Returns a copy of `Area` with the specified field updated. | N/A |
| `koto::Constraint` | Struct | `include/koto/layout.hpp` | 95 | 1D layout sizing rule with `Type {Fixed, Fill, Percent}` and `int value`. | `value = 0` |
| `Constraint::fixed(int n)` | Static method | `include/koto/layout.hpp` | 101 | Returns `{Type::Fixed, n < 0 ? 0 : n}`. | N/A |
| `Constraint::fill()` | Static method | `include/koto/layout.hpp` | 102 | Returns `{Type::Fill, 0}`. | N/A |
| `Constraint::percent(int n)` | Static method | `include/koto/layout.hpp` | 103 | Returns `{Type::Percent, n < 0 ? 0 : n}`. | N/A |
| `koto::LayoutResult` | Class | `include/koto/layout.hpp` | 109 | Small-buffer-optimized container storing up to `InlineCapacity = 16` `Area`s on the stack before spilling to `std::vector<Area>`. | `m_size = 0` |
| `LayoutResult::push_back` / `size` / `empty` / `operator[]` / `at` / `begin` / `end` / `cbegin` / `cend` / `front` / `back` / `to_vector` | Methods | `include/koto/layout.hpp` | 116–177 | Container accessors and explicit `std::vector<Area>` conversion (`at` returns `{0,0,0,0}` if out of bounds). | N/A |
| `koto::Grid` | Class | `include/koto/layout.hpp` | 185 | 2D matrix of `Area` cells supporting `at(r, c)`, `operator()(r, c)`, `operator[r, c]`, `operator[](r)[c]`, and `for_each`. | `m_rows = 0, m_cols = 0` |
| `Layout::vertical(Area, std::span<const Constraint>)` | Function | `include/koto/layout.hpp` | 244 | Splits `area` top-to-bottom into `LayoutResult`. | N/A |
| `Layout::horizontal(Area, std::span<const Constraint>)` | Function | `include/koto/layout.hpp` | 245 | Splits `area` left-to-right into `LayoutResult`. | N/A |
| `Layout::vertical<N>(Area, const Constraint (&)[N])` / `std::array<Constraint, N>` | Template functions | `include/koto/layout.hpp` | 249, 261 | Compile-time array overloads returning `std::array<Area, N>` for structured bindings. | N/A |
| `Layout::horizontal<N>(Area, const Constraint (&)[N])` / `std::array<Constraint, N>` | Template functions | `include/koto/layout.hpp` | 273, 285 | Compile-time array overloads returning `std::array<Area, N>` for structured bindings. | N/A |
| `Layout::center(Area area, int width, int height)` | Function | `include/koto/layout.hpp` | 296 | Returns centered `Area` clamped to `area.width` and `area.height`. | N/A |
| `Layout::grid(Area, row_constraints, col_constraints)` | Function | `include/koto/layout.hpp` | 298 | Partitions `area` into a 2D `Grid` using row and column constraint spans. | N/A |
| `Layout::grid_uniform(Area area, size_t rows, size_t cols)` | Function | `include/koto/layout.hpp` | 302 | Creates a `rows x cols` `Grid` where all rows and columns use `Constraint::fill()`. | N/A |
| `Layout::grid_fixed(Area area, size_t rows, size_t cols, int cell_width, int cell_height)` | Function | `include/koto/layout.hpp` | 304 | Creates a `rows x cols` `Grid` where each cell has fixed `cell_width` and `cell_height`. | N/A |

### `include/koto/utf8.hpp` & `include/koto/io.hpp` & `include/koto/widget.hpp`

| Symbol | Kind | Header | Line | One-line purpose | Default values |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `koto::Utf8Buffer` | Struct | `include/koto/utf8.hpp` | 8 | Value-returned 5-byte UTF-8 encoded buffer (`char data[5]`, `uint8_t size`, `view()`, `c_str()`). | `data = {}`, `size = 0` |
| `koto::next_codepoint(std::string_view& in)` | Function | `include/koto/utf8.hpp` | 17 | Decodes next UTF-32 codepoint, advances `in`, and returns `U'�'` on invalid bytes. | N/A |
| `koto::encode_utf8(char32_t code)` | Function | `include/koto/utf8.hpp` | 20 | Encodes `code` into a `Utf8Buffer`. | N/A |
| `koto::next_codepoint(const char*& ptr, const char* end)` | Function | `include/koto/utf8.hpp` | 23 | Pointer-advancing overload of `next_codepoint`. | N/A |
| `koto::encode_utf8(char32_t code, char* utf8)` | Function | `include/koto/utf8.hpp` | 24 | Writes 1–4 UTF-8 bytes into `utf8` and returns byte count. | N/A |
| `koto::Grapheme` | Struct | `include/koto/utf8.hpp` | 26 | Inline extended grapheme cluster (`char data[36]`, `uint8_t size`, `uint8_t width`, `char32_t base_cp`). | `data = {' ', '\0'}`, `size = 1`, `width = 1`, `base_cp = U' '` |
| `koto::next_grapheme(std::string_view& in)` | Function | `include/koto/utf8.hpp` | 130 | Advances `in` by one extended grapheme cluster (ZWJ, VS15/VS16, skin tones, flags, keycaps, combining marks). | N/A |
| `koto::prev_grapheme_offset(text, cursor) noexcept` | Function | `include/koto/utf8.hpp` | 133 | Returns byte offset of the grapheme cluster preceding `cursor` (or `0`). | N/A |
| `koto::next_grapheme_offset(text, cursor) noexcept` | Function | `include/koto/utf8.hpp` | 136 | Returns byte offset of the grapheme cluster end after `cursor` (or `text.size()`). | N/A |
| `koto::visual_col_to_byte_offset(text, target_visual_col) noexcept` | Function | `include/koto/utf8.hpp` | 139 | Maps a 0-based visual display column to its UTF-8 byte offset in `text`. | N/A |
| `koto::is_unicode_wide(char32_t cp) noexcept` | Function | `include/koto/utf8.hpp` | 141 | Returns `true` if `cp` is in `UNICODE_WIDE_RANGES` (Unicode 16.0 UAX #11 / UTS #51). | N/A |
| `koto::is_unicode_zero_width(char32_t cp) noexcept` | Function | `include/koto/utf8.hpp` | 142 | Returns `true` if `cp` is `0`, `0x200B`, `0xFEFF`, or in `UNICODE_ZERO_RANGES`. | N/A |
| `koto::codepoint_width(char32_t cp)` | Function | `include/koto/utf8.hpp` | 143 | Returns display column width (`0`, `1`, or `2`) of single codepoint `cp`. | N/A |
| `koto::text_width(std::string_view text)` | Function | `include/koto/utf8.hpp` | 144 | Sums visual widths of all graphemes in `text` (expanding `\t` to 4-column tab stops). | N/A |
| `koto::write_all(int fd, std::string_view data)` | Function (`inline`) | `include/koto/io.hpp` | 13 | Writes all bytes in `data` to `fd`, retrying on `EINTR` and returning `std::expected<void, int>`. | N/A |
| `koto::Widget` | Abstract class | `include/koto/widget.hpp` | 8 | Base class for all widgets with virtual destructor and `virtual void draw(Frame& frame, Area area) = 0`. | N/A |

### Widget & State Headers (`box.hpp`, `button.hpp`, `fill.hpp`, `gauge.hpp`, `hbox.hpp`, `list.hpp`, `rule.hpp`, `sparkline.hpp`, `tabs.hpp`, `text.hpp`, `text_area.hpp`, `text_input.hpp`, `vbox.hpp`)

| Symbol | Kind | Header | Line | One-line purpose | Default values |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `koto::BorderStyle` | Enum class | `include/koto/box.hpp` | 11 | Border line style (`None`, `Single`, `Double`, `Rounded`, `Thick`). | N/A |
| `koto::TitleAlign` | Enum class | `include/koto/box.hpp` | 20 | Horizontal alignment for widget titles (`Left`, `Center`, `Right`). | N/A |
| `koto::Borders` | Struct | `include/koto/box.hpp` | 27 | Per-edge border toggles `{top, bottom, left, right}`. | All `true` |
| `koto::Box` | Widget class | `include/koto/box.hpp` | 32 | Draws borders, optional interior fill, and aligned title (`fg`, `bg`, `title_fg`, `title_bg`, `filled`, `border_style`, `borders`, `title`, `title_align`, `focused`, `focused_border_fg`, `focused_border_style`, `id`, `tag`, `draw`). | `m_style = BorderStyle::Single`, `m_title_align = TitleAlign::Center`, `m_focused_border_fg = Color::Cyan`, `m_focused_border_style = BorderStyle::Thick` (only if `focused_border_style()` called) |
| `koto::Button` | Widget class | `include/koto/button.hpp` | 17 | Clickable and focusable button (`label`, `border_style`, `fg`, `bg`, `label_fg`, `focused`, `focused_fg`, `focused_bg`, `focused_border_fg`, `focused_label_fg`, `id`, `focus`, `on_click`, `draw`). | `m_border_style = BorderStyle::Rounded`, `m_fg = Color::DarkGray`, `m_bg = COLOR_DEFAULT`, `m_label_fg = Color::LightGray`, `m_focused_fg = Color::Cyan`, `m_focused_bg = Color::DarkGray`, `m_focused_border_fg = Color::Cyan`, `m_focused_label_fg = Color::White` |
| `koto::Fill` | Widget class | `include/koto/fill.hpp` | 14 | Fills an `Area` with character `char_code` and `fg`/`bg` colors (`bg`, `fg`, `char_code`, `id`, `draw`). | `m_ch = U' '`, `m_fg = COLOR_DEFAULT`, `m_bg = COLOR_DEFAULT` |
| `koto::Clear` | Widget class | `include/koto/fill.hpp` | 34 | Resets an `Area` to spaces with `COLOR_DEFAULT` foreground and background (`id`, `draw`). | `m_tag_id = Id::none()` |
| `koto::Gauge` | Widget class | `include/koto/gauge.hpp` | 11 | Horizontal progress bar with 1/8th fractional blocks and centered label (`ratio`, `percent`, `label`, `filled_char`, `empty_char`, `use_fractional_blocks`, `fg`, `bg`, `filled_fg`, `filled_bg`, `empty_fg`, `empty_bg`, `label_fg`, `draw`). | `m_ratio = 0.0f`, `m_fractional = true`, `m_filled_ch = U'█'`, `m_empty_ch = U' '`, `m_filled_fg = Color::Emerald`, `m_empty_fg = Color::DarkGray`, `m_label_fg = Color::White` |
| `koto::Hbox` | Widget class | `include/koto/hbox.hpp` | 11 | Container owning child widgets via `add<T>(Constraint, Args&&...)` and laying them out horizontally. | Empty children |
| `koto::Vbox` | Widget class | `include/koto/vbox.hpp` | 11 | Container owning child widgets via `add<T>(Constraint, Args&&...)` and laying them out vertically. | Empty children |
| `koto::ListNavigation` | Enum class | `include/koto/list.hpp` | 18 | List boundary behavior (`Clamp`, `Wrap`). | `Clamp` |
| `koto::ListState` | State struct | `include/koto/list.hpp` | 24 | Holds `items`, `selected`, `scroll`, `focused`, `navigation`, `on_change`, `rendered_area`, `rendered_inner`, and selection/input methods. | `selected = 0`, `scroll = 0`, `focused = true`, `navigation = ListNavigation::Clamp` |
| `koto::List` | Widget class | `include/koto/list.hpp` | 52 | Renders a scrollable `ListState` with optional border and highlight symbol (`fg`, `bg`, `highlight_fg`, `highlight_bg`, `border_style`, `highlight_symbol`, `title`, `title_fg`, `title_bg`, `title_align`, `overflow`, `focused`, `focused_border_fg`, `id`, `draw`). | `m_border_style = BorderStyle::None`, `m_highlight_symbol = ""`, `m_title_align = TitleAlign::Left`, `m_overflow = TextOverflow::Ellipsis`, `m_focused_border_fg = Color::Cyan` |
| `koto::Rule` | Widget class | `include/koto/rule.hpp` | 12 | Horizontal divider line with optional padded title (`title`, `title_align`, `style`, `fg`, `bg`, `title_fg`, `title_bg`, `draw`). | `m_title_align = TitleAlign::Center`, `m_style = BorderStyle::Single`, colors `COLOR_DEFAULT` |
| `koto::Sparkline` | Widget class | `include/koto/sparkline.hpp` | 12 | Single-row vertical bar chart using 8 Unicode block levels (`data(span)`, `data(vector)`, `max`, `align_right`, `fg`, `bg`, `draw`). | `m_max = 0` (auto), `m_align_right = true`, `m_fg = Color::Cyan`, `m_bg = COLOR_DEFAULT` |
| `koto::TabsStyle` | Enum class | `include/koto/tabs.hpp` | 19 | Visual style for `Tabs` (`Bar`, `Pill`, `Underline`). | `Bar` |
| `koto::TabRect` | Struct | `include/koto/tabs.hpp` | 26 | Hit rectangle `{start_col, end_col, start_row, end_row}` recorded per tab in `TabsState::rendered_tabs`. | All `0` |
| `koto::TabsState` | State struct | `include/koto/tabs.hpp` | 34 | Holds `titles`, `selected`, `focused`, `on_change`, `rendered_tabs`, and tab selection/input methods. | `selected = 0`, `focused = true` |
| `koto::Tabs` | Widget class | `include/koto/tabs.hpp` | 57 | Renders stateful (`TabsState&`) or stateless (`vector<string>`) tab strip (`selected`, `style`, `separator`, `separator_fg`, `active_fg`, `active_bg`, `inactive_fg`, `inactive_bg`, `border_style`, `title`, `title_align`, `focused`, `focused_border_fg`, `id`, `draw`). | `m_style = TabsStyle::Bar`, `m_separator = "│"`, `m_separator_fg = Color::DarkGray`, `m_active_fg = Color::Black`, `m_active_bg = Color::Cyan`, `m_inactive_fg = Color::White`, `m_inactive_bg = COLOR_DEFAULT`, `m_border_style = BorderStyle::None`, `m_title_align = TitleAlign::Left`, `m_focused_border_fg = Color::Cyan` |
| `koto::TextWrap` / `TextOverflow` / `TextAlign` | Enum classes | `include/koto/text.hpp` | 11, 17, 23 | `TextWrap {None, Wrap}`, `TextOverflow {Clip, Ellipsis}`, `TextAlign {Left, Center, Right}`. | N/A |
| `koto::Text` | Widget class | `include/koto/text.hpp` | 30 | Renders single- or multi-line UTF-8 text (`fg`, `bg`, `wrap`, `clip`, `ellipsis`, `overflow`, `align`, `bold`, `dim`, `italic`, `underline`, `reverse`, `attr`, `draw`). | `m_align = TextAlign::Left`, `m_overflow = TextOverflow::Clip`, `m_text_wrap = TextWrap::None`, `m_attrs = 0` |
| `koto::CursorColumnMode` / `ScrollMode` | Enum classes | `include/koto/text_area.hpp` | 14, 20 | `CursorColumnMode {Clamp, Sticky}`, `ScrollMode {Clamp, Margin}`. | `Sticky`, `Margin` |
| `koto::TextAreaState` | State struct | `include/koto/text_area.hpp` | 26 | Multi-line editor state holding `text`, `cursor_row`, `cursor_col`, `preferred_col`, `scroll_row`, `scroll_col`, `scroll_margin`, `show_line_number`, `focused`, `cursor_mode`, `scroll_mode`, and editing methods. | `text = {""}`, `scroll_margin = 1`, `show_line_number = true`, `focused = true`, `cursor_mode = CursorColumnMode::Sticky`, `scroll_mode = ScrollMode::Margin` |
| `koto::TextArea` | Widget class | `include/koto/text_area.hpp` | 66 | Renders `TextAreaState` with optional border and line-number gutter (`border_style`, `title`, `title_align`, `line_numbers`, `fg`, `bg`, `line_number_fg`, `focused`, `focused_border_fg`, `id`, `cursor_mode`, `scroll_mode`, `draw`). | `m_border_style = BorderStyle::Single`, `m_title_align = TitleAlign::Left`, `m_show_line_numbers = true`, `m_line_number_fg = Color::DarkGray`, `m_focused = true`, `m_focused_border_fg = Color::Cyan` |
| `koto::TextInputState` | State struct | `include/koto/text_input.hpp` | 16 | Single-line editor state holding `value`, `cursor`, `scroll_offset`, `focused`, `on_key_handler`, `input_filter`, and editing methods. | `cursor = 0` (or `initial_text.size()`), `scroll_offset = 0`, `focused = true` |
| `koto::TextInput` | Widget class | `include/koto/text_input.hpp` | 55 | Renders `TextInputState` with horizontal scroll and placeholder (`placeholder`, `title`, `border_style`, `fg`, `bg`, `placeholder_fg`, `focused_border_fg`, `focused`, `id`, `draw`). | `m_border_style = BorderStyle::Single`, `m_placeholder_fg = 0x00888888`, `m_focused_border_fg = 0x0055FFFF`, `m_focused = false` |

## Widget Key and Mouse Bindings (`src/*.cpp`)

| Widget / State | Input (`KeyEvent` / `MouseEvent` / Paste) | Condition | Action Performed | Source Location |
| :--- | :--- | :--- | :--- | :--- |
| `Button` | `k.is(Key::Enter)` or `k.ch == ' '` | Registered via `.focus(fm)`, `.id(...)`, and `.on_click(cb)` | Invokes `m_on_click()` and returns `true`. | `src/button.cpp:128-135` |
| `Button` | `m.is_left_press()` (`MouseAction::Press` + `MouseButton::Left`) | Registered via `.focus(fm)`, `.id(...)`, and `.on_click(cb)` | Focuses button in `FocusManager`, invokes `m_on_click()`, and returns `true`. | `src/button.cpp:136-143`, `src/focus.cpp:673-681` |
| `ListState` | `key.ch == 'j'` or `key.is(Key::Down)` | `focused && !items.empty()` | Calls `select_next()` (clamps or wraps per `navigation`, fires `on_change`) and returns `true`. Note: `key.ch == 'j'` does not check `ctrl`/`alt` modifiers. | `src/list.cpp:131-135` |
| `ListState` | `key.ch == 'k'` or `key.is(Key::Up)` | `focused && !items.empty()` | Calls `select_prev()` and returns `true`. | `src/list.cpp:136-140` |
| `ListState` | `key.ch == 'g'` or `key.is(Key::Home)` | `focused && !items.empty()` | Calls `select_first()` (`set_selected(0)`) and returns `true`. | `src/list.cpp:141-145` |
| `ListState` | `key.ch == 'G'` or `key.is(Key::End)` | `focused && !items.empty()` | Calls `select_last()` (`set_selected(size - 1)`) and returns `true`. | `src/list.cpp:146-150` |
| `ListState` | `mouse.button == MouseButton::WheelUp` | `!items.empty()` | Calls `select_prev()` and returns `true` (does not require `focused`). | `src/list.cpp:159-163` |
| `ListState` | `mouse.button == MouseButton::WheelDown` | `!items.empty()` | Calls `select_next()` and returns `true` (does not require `focused`). | `src/list.cpp:164-168` |
| `ListState` | `MouseAction::Press` + `MouseButton::Left` | `rendered_inner.contains(mouse.row, mouse.col)` and valid item index | Calls `set_selected(scroll + (mouse.row - rendered_inner.row))` and returns `true`. | `src/list.cpp:170-182` |
| `TabsState` | `key.is(Key::Right)` or `key.ch == 'l'` | `focused && !titles.empty()` | Calls `select_next()` (always wraps circularly, fires `on_change`) and returns `true`. | `src/tabs.cpp:100-104` |
| `TabsState` | `key.is(Key::Left)` or `key.ch == 'h'` | `focused && !titles.empty()` | Calls `select_prev()` (always wraps circularly, fires `on_change`) and returns `true`. | `src/tabs.cpp:105-109` |
| `TabsState` | `key.is(Key::Home)` | `focused && !titles.empty()` | Calls `select_first()` and returns `true`. | `src/tabs.cpp:110-114` |
| `TabsState` | `key.is(Key::End)` | `focused && !titles.empty()` | Calls `select_last()` and returns `true`. | `src/tabs.cpp:115-119` |
| `TabsState` | `key.ch >= '1' && key.ch <= '9'` | `focused && !titles.empty() && !key.ctrl && !key.alt` and `(key.ch - '1') < titles.size()` | Calls `set_selected(key.ch - '1')` and returns `true`. | `src/tabs.cpp:122-130` |
| `TabsState` | `MouseButton::WheelUp` / `MouseButton::WheelDown` | `!titles.empty()` | Calls `select_prev()` / `select_next()` and returns `true`. | `src/tabs.cpp:139-148` |
| `TabsState` | `MouseAction::Press` + `MouseButton::Left` | Coordinates inside `rendered_tabs[i]` | Calls `set_selected(i)` and returns `true`. | `src/tabs.cpp:150-162` |
| `TextInputState` | `on_key_handler(*this, key)` | `focused && on_key_handler` returns `true` | Consumes key via user pre-hook before default bindings. | `src/text_input.cpp:185-188` |
| `TextInputState` | `key.is(Key::Left)` / `key.is(Key::Right)` | `focused` | Calls `move_left()` / `move_right()` (returns `false` at start/end boundary!). | `src/text_input.cpp:191-198` |
| `TextInputState` | `key.is(Key::Home)` or `key.ctrl && (ch == 'A' \|\| ch == 'a')` | `focused` | Calls `move_home()` (returns `true` only if `cursor != 0`). | `src/text_input.cpp:199-202` |
| `TextInputState` | `key.is(Key::End)` or `key.ctrl && (ch == 'E' \|\| ch == 'e')` | `focused` | Calls `move_end()` (returns `true` only if `cursor != value.size()`). | `src/text_input.cpp:203-206` |
| `TextInputState` | `key.ctrl && (ch == 'U' \|\| ch == 'u')` | `focused` | If `!value.empty()`, calls `clear()` and returns `true`; else returns `false`. | `src/text_input.cpp:209-217` |
| `TextInputState` | `key.ctrl && (ch == 'W' \|\| ch == 'w')` | `focused` | Calls `delete_word()` (deletes trailing spaces then non-spaces before cursor). | `src/text_input.cpp:218-221` |
| `TextInputState` | `key.is(Key::Backspace)` / `key.is(Key::Delete)` | `focused` | Calls `backspace()` / `delete_forward()` by one grapheme cluster. | `src/text_input.cpp:224-231` |
| `TextInputState` | `!key.ctrl && !key.alt && key.is_char()` | `focused` | Calls `insert(static_cast<char32_t>(key.ch))` (subject to `input_filter`). | `src/text_input.cpp:234-237` |
| `TextInputState` | `PasteEvent` (via `FocusManager`) | `HasPasteHandler<TextInputState>` | Calls `state.insert(str)` and returns `true`. | `include/koto/focus.hpp:161-166` |
| `TextAreaState` | `key.is(Key::Up)` / `Down` / `Left` / `Right` | `focused` | Calls `move_up()` / `move_down()` / `move_left()` / `move_right()` and always returns `true`. | `src/text_area.cpp:338-357` |
| `TextAreaState` | `key.is(Key::Home)` or `key.ctrl && (ch == 'A' \|\| ch == 'a')` | `focused` | Calls `move_home()` and returns `true`. | `src/text_area.cpp:358-362` |
| `TextAreaState` | `key.is(Key::End)` or `key.ctrl && (ch == 'E' \|\| ch == 'e')` | `focused` | Calls `move_end()` and returns `true`. | `src/text_area.cpp:363-367` |
| `TextAreaState` | `key.is(Key::Backspace)` / `key.is(Key::Delete)` | `focused` | Calls `backspace()` / `delete_forward()` (joins lines across boundaries) and returns `true`. | `src/text_area.cpp:368-377` |
| `TextAreaState` | `key.is(Key::Enter)` | `focused` | Calls `newline()` (splits current line at `cursor_col`) and returns `true`. | `src/text_area.cpp:378-382` |
| `TextAreaState` | `key.is(Key::Tab)` | `focused` | Calls `insert("    ")` (4 spaces) and returns `true` when invoked directly (note: `FocusManager::handle_key` intercepts `Key::Tab` for focus cycling before calling target `handle_key` unless handled via custom interceptor). | `src/text_area.cpp:383-387`, `src/focus.cpp:551-558` |
| `TextAreaState` | `!key.ctrl && !key.alt && key.is_char()` | `focused` | Calls `insert(static_cast<char32_t>(key.ch))` and returns `true`. | `src/text_area.cpp:388-392` |
| `TextAreaState` | `MouseButton::WheelUp` / `MouseButton::WheelDown` | Any | Calls `move_up()` / `move_down()` and returns `true` (does not position cursor on left click). | `src/text_area.cpp:397-410` |
| `TextAreaState` | `PasteEvent` (via `FocusManager`) | `HasPasteHandler<TextAreaState>` | Calls `state.insert(str)` and returns `true`. | `include/koto/focus.hpp:161-166` |
| `Box`, `Fill`, `Clear`, `Gauge`, `Hbox`, `Vbox`, `Rule`, `Sparkline`, `Text` | None | Stateless widgets | Do not implement `handle_event` or `handle_mouse` directly (`Box`, `Fill`, `Clear` can be tagged with `.id(...)` and registered in `FocusManager::add(Id, ...)`). | `include/koto/*.hpp` |

## Phase 2 — Documentation Site Page Plan (`src/content/docs/`)

| Page Path | One-Line Scope |
| :--- | :--- |
| `api/app.md` | Reference for `koto::App` lifecycle, event callbacks, `use_focus`, headless `draw_frame`/`dispatch_event`, and `run()`. |
| `api/terminal.md` | Reference for `koto::Terminal`, `TerminalError`, `TerminalErrorInfo`, raw mode RAII, signal handlers, and protocol toggles. |
| `api/frame.md` | Reference for `koto::Frame` double-buffering, `set_cell` continuation rules, hit buffer tagging, bounds harvesting, and snapshots. |
| `api/cell-color.md` | Reference for `koto::Cell`, `TextAttr`, `Color`, `ColorMode`, `ColorParseError`, hex/name parsing, `lerp`, and ANSI quantization. |
| `api/events.md` | Reference for `Key`, `KeyEvent`, `MouseButton`, `MouseAction`, `MouseEvent`, `ResizeEvent`, `PasteEvent`, `TickEvent`, `Event`, and `EventLoop`. |
| `api/focus-manager.md` | Reference for `FocusManager`, `FocusableState`, `HasMouseHandler`, `HasPasteHandler`, `FocusTarget`, `IdScope`, `FocusNavPolicy`, `Direction`, and `ScopeMode`. |
| `api/id.md` | Reference for `koto::Id`, `TAG_NONE`, and `fnv1a_32` compile-time and runtime hashing overloads. |
| `api/layout.md` | Reference for `Area`, `Constraint`, `LayoutResult`, `Grid`, and `Layout::vertical`, `horizontal`, `center`, `grid`, `grid_uniform`, and `grid_fixed`. |
| `api/utf8.md` | Reference for `Utf8Buffer`, `Grapheme`, `next_codepoint`, `encode_utf8`, `next_grapheme`, grapheme offset helpers, and Unicode 16.0 width functions. |
| `api/io.md` | Reference for `koto::write_all` (`include/koto/io.hpp`), `koto::Widget` (`include/koto/widget.hpp`), and umbrella header `include/koto/koto.hpp`. |
| `widgets/box.md` | Widget reference for `koto::Box`, `BorderStyle`, `TitleAlign`, and `Borders`. |
| `widgets/button.md` | Widget reference for `koto::Button` styling, `FocusManager` registration, and keyboard/mouse click handling. |
| `widgets/fill.md` | Widget reference for `koto::Fill` and `koto::Clear` (`include/koto/fill.hpp`). |
| `widgets/gauge.md` | Widget reference for `koto::Gauge` 1/8th fractional sub-block progress bars and background-preserving labels. |
| `widgets/hbox.md` | Widget reference for `koto::Hbox` horizontal widget container (`include/koto/hbox.hpp`). |
| `widgets/list.md` | Widget reference for `koto::List`, `ListState`, and `ListNavigation`. |
| `widgets/rule.md` | Widget reference for `koto::Rule` horizontal dividers and aligned titles. |
| `widgets/sparkline.md` | Widget reference for `koto::Sparkline` single-row 8-level vertical bar charts. |
| `widgets/tabs.md` | Widget reference for `koto::Tabs`, `TabsState`, `TabsStyle`, and `TabRect`. |
| `widgets/text.md` | Widget reference for `koto::Text`, `TextAlign`, `TextOverflow`, and `TextWrap`. |
| `widgets/text-area.md` | Widget reference for `koto::TextArea`, `TextAreaState`, `CursorColumnMode`, and `ScrollMode`. |
| `widgets/text-input.md` | Widget reference for `koto::TextInput` and `TextInputState`. |
| `widgets/vbox.md` | Widget reference for `koto::Vbox` vertical widget container (`include/koto/vbox.hpp`). |
| `concepts/immediate-mode.md` | Explains how `libkoto` separates transient `Widget` builders from persistent `state struct` objects each frame. |
| `concepts/frames-diff-renderer.md` | Explains front/back `cell` buffers, wide continuation cells, SGR state tracking, and DEC Mode 2026 synchronized updates. |
| `concepts/ids-hit-buffer.md` | Explains 32-bit FNV-1a `id` tokens, the `Frame` `hit buffer`, and automatic bounding-box harvesting. |
| `concepts/events-input.md` | Explains `EventLoop` polling, `FIONREAD` burst draining, 35 ms escape and 500 ms paste deadlines, and event dispatch. |
| `concepts/focus-management.md` | Explains the `FocusManager` 1D `focus ring`, 2D spatial navigation, `scope` namespacing, and `modal scope` trapping. |
| `concepts/layout.md` | Explains `Area` geometry, `Constraint` resolution, 16-element stack `LayoutResult`, and 2D `Grid` partitioning. |
| `architecture/event-loop.md` | Architecture walkthrough and Mermaid sequence/flowchart of `App::run()`, `EventLoop::poll()`, and the POSIX self-pipe. |
| `architecture/focus-system.md` | Architecture walkthrough and Mermaid diagram of `FocusManager` target registration, geometry sync, key/mouse routing, and modal scopes. |
| `architecture/rendering-pipeline.md` | Architecture walkthrough and Mermaid diagram of `App::draw_frame()`, `Frame::set_cell()`, diff scanning, and `write_all()`. |
| `guides/layout-recipes.md` | Task guide for building app shells, sidebars, centered dialogs, and 2D grids with `Layout` and `Constraint`. |
| `guides/keyboard.md` | Task guide for handling keys, modifiers, Kitty CSI-u disambiguation, and custom `FocusKeyInterceptor` hooks. |
| `guides/mouse.md` | Task guide for enabling SGR mouse capture, hit-testing with `Id` tags, wheel scrolling, and drag/hover modes. |
| `guides/modal-dialogs.md` | Task guide for trapping focus and shielding background clicks with `FocusManager::push_scope`/`pop_scope` and `IdScope`. |
| `guides/colors.md` | Task guide for using 24-bit RGB, hex strings, `Color::lerp` gradients, and `ColorMode` quantization. |
| `guides/unicode.md` | Task guide for rendering CJK/emoji text, measuring `text_width`, and editing grapheme clusters safely. |
| `guides/headless-testing.md` | Task guide for writing TTY-free unit tests using `Frame(rows, cols)`, `EventLoop::parse`/`feed`, and `App::draw_frame(false)`. |
| `getting-started/requirements.md` | Compiler, CMake, OS, and terminal requirements verified from `CMakeLists.txt` and `.github/workflows/ci.yml`. |
| `getting-started/installation.md` | Installing and linking `koto::koto` via CMake `add_subdirectory`, `FetchContent`, `find_package`, or `pkg-config`. |
| `getting-started/first-app.md` | Complete minimal `koto` application with layout, state struct, focus, and error handling. |
| `getting-started/building-running.md` | Building the library, examples, and CTest headless suites with Release, Debug, and sanitizer flags. |
| `deep-dives/performance.md` | Deep dive into 64-byte `Cell` layout, `LayoutResult` stack buffer, `FIONREAD` batching, and zero-dirty-cell frame skipping. |
| `deep-dives/troubleshooting.md` | Diagnosing `NotATty`, `NO_COLOR`/`COLORTERM` quantization, `SIGKILL` terminal reset, and non-Kitty key bindings. |
| `deep-dives/unicode.md` | Deep dive into RFC 3629 UTF-8 decoding, `Grapheme` segmentation (`char data[36]`), and 2-column continuation invariants. |
| `limitations-faq.md` | Source-verified platform, protocol, widget, and threading limitations and frequently asked questions. |
| `changelog.md` | Commit-by-commit history derived strictly from `git log` up to `51c6c1dc7d25cb805ca620ef9b8af699941e14f1`. |
