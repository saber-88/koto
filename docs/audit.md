# libkoto Documentation Self-Audit Report (`audit.md`)

- **Repository**: `https://github.com/saber-88/koto.git`
- **Verified Commit (`git rev-parse HEAD`)**: `51c6c1dc7d25cb805ca620ef9b8af699941e14f1`
- **Documentation Site Root**: `src/content/docs/` (48 Astro Starlight Markdown pages)
- **Manifest File**: `api-manifest.md`

## 1. Automated Verification Summary

| Check | Result |
| :--- | :--- |
| **Total Astro Starlight Pages Generated** | `48` pages across `api/` (10), `widgets/` (13), `concepts/` (6), `architecture/` (3), `guides/` (7), `getting-started/` (4), `deep-dives/` (3), `limitations-faq.md`, and `changelog.md` |
| **Public Headers Covered (`include/koto/*.hpp`)** | `22 / 22` (`app.hpp`, `box.hpp`, `button.hpp`, `cell.hpp`, `color.hpp`, `event.hpp`, `fill.hpp`, `focus.hpp`, `frame.hpp`, `gauge.hpp`, `hbox.hpp`, `id.hpp`, `io.hpp`, `koto.hpp`, `layout.hpp`, `list.hpp`, `rule.hpp`, `sparkline.hpp`, `tabs.hpp`, `terminal.hpp`, `text.hpp`, `text_area.hpp`, `text_input.hpp`, `utf8.hpp`, `vbox.hpp`, `widget.hpp` — 25 header files total, 100% covered) |
| **Public Widgets Covered** | `13 / 13` (`Box`, `Button`, `Fill`, `Clear`, `Gauge`, `Hbox`, `List`, `Rule`, `Sparkline`, `Tabs`, `Text`, `TextArea`, `TextInput`, `Vbox`) |
| **C++23 Code Blocks Compiled & Linked** | `41 / 41` compiled and linked against `libkoto.a` with `-std=c++23 -Wall -Wextra -Wconversion -Wshadow -Werror` (`0` warnings, `0` errors) |
| **Render & Headless Blocks Executed** | `24 / 24` executed with exit code `0` |
| **Frontmatter & Link Audit** | `48 / 48` pages have valid frontmatter (`description < 155` chars), zero top-level `# ` headings, root-absolute links (`/group/page/`), zero emojis, zero prose exclamation marks, and 2–4 `See also` links |

## 2. Claims Verified Against Source (`include/` and `src/`)

1. **`Cell` and `Grapheme` binary layout (`include/koto/cell.hpp:29-75`, `include/koto/utf8.hpp:26-32`)**:
   - Verified `Grapheme::data` is `char data[36]` (up to `35` UTF-8 bytes + null terminator) and `sizeof(Grapheme) == 44`.
   - Verified `Cell` has `uint8_t width = 1`, `bool continuation = false`, `uint8_t attrs = 0`, `uint8_t reserved[5] = {}`, and `static_assert(sizeof(Cell) == 64)`.
2. **`App` lifecycle and event dispatch order (`include/koto/app.hpp:12-51`, `src/app.cpp:12-329`)**:
   - Verified `App` constructor does not enter raw mode; `App::run()` checks `m_term.is_tty()`, calls `m_term.enter_raw_mode()`, hides the cursor, and restores the terminal on exit.
   - Verified `App::dispatch_event` order for `ResizeEvent`, `PasteEvent`, `MouseEvent`, and `KeyEvent`, including `MouseEvent::target` enrichment via `m_frame.hit_at(m.row, m.col)`.
   - Verified `App::run()` wait timeout calculation (`0` ms when pending/dirty, `0..16` ms when tick handlers are set, `35` ms cap when `m_loop.has_pending()` is true, `-1` ms when idle) and user-space burst draining via `while (m_running && m_loop.has_pending_event())`.
3. **`EventLoop` timeouts and protocol decoding (`include/koto/event.hpp:14-204`, `src/event.cpp:16-1082`)**:
   - Verified `35` ms short-sequence deadline (`m_escape_deadline`) and `500` ms / `4 MB` bracketed paste deadline (`m_paste_deadline`).
   - Verified Kitty CSI-u parsing (`[<cp>;<mod>u`), suppression of key release events (`event_type == 3`) and standalone modifier keys (`57358..57363`, `57441..57454`), and uppercase normalization of `Ctrl + 'a'..'z'`.
   - Verified SGR 1006 mouse parsing (`[<btn;col;rowM/m`), 1-based to 0-based coordinate conversion, and modifier bitmasks (`Shift = 4`, `Alt = 8`, `Ctrl = 16`, `Motion = 32`).
4. **`FocusManager` ring, spatial scoring, and modal trapping (`include/koto/focus.hpp:27-313`, `src/focus.cpp:50-803`)**:
   - Verified `FocusManager::handle_key` execution order: `m_key_interceptor` -> `BackTab`/`Tab` -> `AltArrows` -> active target `handle_key` -> `SpatialArrows`.
   - Verified 2D spatial distance formula `score = primary_dist + 2 * secondary_dist`.
   - Verified `ScopeMode::Modal` saves focus on `push_scope`, isolates 1D/2D navigation and shields mouse clicks, and prunes modal targets via `std::erase_if` while restoring saved focus on `pop_scope`.
5. **`Layout` constraint solver and `LayoutResult` (`include/koto/layout.hpp:10-305`, `src/layout.cpp:8-167`)**:
   - Verified `LayoutResult::InlineCapacity = 16` stack array before heap fallback, `Constraint::fixed` / `percent` / `fill` calculation, remainder distribution (`fill_idx < fill_remainder ? 1 : 0`), and compile-time array overloads returning `std::array<Area, N>`.
6. **Widget defaults and key/mouse bindings (`include/koto/*.hpp`, `src/*.cpp`)**:
   - Verified every constructor, fluent setter, default member initializer, and `handle_event` / `handle_mouse` branch across all 13 widgets and 4 state structs.

## 3. Open Questions Marked `TODO(verify)` in Documentation Pages

| Page | Line / Section | `TODO(verify)` Question |
| :--- | :--- | :--- |
| `src/content/docs/widgets/text-input.md` | Before `## See also` | `<!-- TODO(verify): Should TextInput::draw() check is_foc instead of m_state.focused at src/text_input.cpp:362 and 428 when calling frame.set_cursor(), matching TextArea::draw() at src/text_area.cpp:679? -->` |
| `src/content/docs/widgets/list.md` | Before `## See also` | `<!-- TODO(verify): Should ListState::handle_event() check !key.ctrl && !key.alt before matching 'j', 'k', 'g', and 'G' at src/list.cpp:131-150? -->` |
| `src/content/docs/widgets/tabs.md` | Before `## See also` | `<!-- TODO(verify): Should TabsState::handle_event() check !key.ctrl && !key.alt before matching 'l' and 'h' at src/tabs.cpp:100-109, matching the check on '1'..'9' at line 122? -->` |
| `src/content/docs/widgets/text-area.md` | Before `## See also` | `<!-- TODO(verify): Since FocusManager::handle_key() intercepts Key::Tab at step 2 (src/focus.cpp:555) before calling the focused target's handle_key() at step 4 (src/focus.cpp:570), is Key::Tab in TextAreaState::handle_event() (src/text_area.cpp:383) intended only for standalone usage or custom FocusKeyInterceptors? -->` |

## 4. Mismatches Found Between Existing `README.md` / `docs/` and Source Code

Where the existing `README.md` or `docs/` disagreed with `include/` and `src/`, the new documentation follows the source code exclusively:

| Existing File | Claim in Old Docs / README | Actual Source Truth (`include/` & `src/`) |
| :--- | :--- | :--- |
| `README.md:24` | Claims `"31-byte grapheme storage for multi-codepoint ZWJ emoji clusters"`. | `Grapheme::data` is `char data[36]` (`include/koto/utf8.hpp:28, 90`), storing up to **35** UTF-8 bytes plus null terminator. |
| `README.md:104`, `docs/getting-started.md:10` | Claims `"Build System: CMake 3.25+"`. | `CMakeLists.txt:1` requires `cmake_minimum_required(VERSION 3.15)`. |
| `README.md:146, 154` | Claims `46` total tests and `7` tests in `test_headless_regressions`. | `tests/test_headless_regressions.cpp:888-906` runs **8** test functions (`ALL 8 REGRESSION TESTS PASSED CLEANLY!`), totaling `47` test functions across the 6 CTest executables. |
| `docs/api/app.md:26, 58, 125` | Documents `void App::wakeup()`. | `App::wakeup()` does not exist (`include/koto/app.hpp`). The thread-safe method on `App` is `void App::request_redraw() noexcept` (`include/koto/app.hpp:37`), while `wakeup()` is on `EventLoop` (`include/koto/event.hpp:187`). |
| `docs/api/app.md:31, 61` | Documents `void App::on_resize(std::function<void(const ResizeEvent&)>)`. | `App::on_resize()` does not exist (`include/koto/app.hpp`). `App::dispatch_event` handles `ResizeEvent` internally (`src/app.cpp:86-92`). |
| `docs/api/app.md:32, 62` | Documents `void App::on_paste(std::function<void(const PasteEvent&)>)`. | `App::on_paste` takes `std::function<void(const std::string&)>` and returns `App&` (`include/koto/app.hpp:22`). |
| `docs/api/app.md:44-45` | Documents `App::rows()` and `App::cols()`. | Neither method exists on `App` (`include/koto/app.hpp:41-51`); `App` provides `Area area() const noexcept`, while `rows()`/`cols()` are on `Frame` and `Terminal`. |
| `docs/api/cell.md:3, 11, 49, 70`, `docs/architecture/rendering-pipeline.md:55-78`, `docs/etc/performance-guide.md:29-38`, `docs/etc/unicode-handling.md:36, 107` | Claims `sizeof(Cell) == 44`, `Grapheme::data` is `char data[24]`, and `Cell::width` is `int`. | Commit `d85b6d4` expanded `Grapheme::data` to `char data[36]`, changed `Cell::width` to `uint8_t`, added `uint8_t reserved[5]`, and added `static_assert(sizeof(Cell) == 64)` (`include/koto/cell.hpp:29-75`). |
| `docs/api/cell.md:155` | Claims `Cell::make_continuation()` sets `width = 0`. | `Cell::make_continuation()` (`include/koto/cell.hpp:64-69`) default-initializes `Cell c{}` (`width = 1`) and sets `c.continuation = true`. |
| `docs/api/color.md:29-56, 74, 92`, `docs/widgets/box.md:143`, `docs/widgets/list.md:130` | Documents `Color::Hex(uint32_t)`, `constexpr Color Color::lerp(Color, Color, float)`, and `uint32_t Color::value() const`. | `Color::Hex` does not exist; `Color::lerp` is non-`constexpr` (uses `std::lround` in `src/color.cpp:112-152`) and returns `uint32_t`; `Color::value` is a `uint32_t` data member, not a method; and `Color::Red` etc. are `static constexpr uint32_t` constants (`include/koto/color.hpp:34-81`). |
| `docs/api/event.md:41-49, 60, 146, 163` | Omits `WheelLeft`/`WheelRight`, claims `MouseEvent::button` defaults to `MouseButton::None`, writes `mouse.target == "btn_submit"`, and claims X10 mouse support. | `MouseButton` includes `WheelLeft = 66` and `WheelRight = 67`; `MouseEvent::button` defaults to `MouseButton::Left` (`include/koto/event.hpp:113`); `mouse.target == "btn_submit"` does not compile without `Id("btn_submit")`; and `src/event.cpp:36-107` only parses SGR 1006 mouse sequences. |
| `docs/api/frame.md:19, 29, 34, 35, 39` | Documents `Frame(int, int, Terminal*)`, `Frame::clear()`, `Frame::tag_at()`, `Area Frame::bounds_of()`, and `Frame::hide_cursor()`. | None of `Frame(int, int, Terminal*)`, `Frame::clear()`, `Frame::tag_at()`, or `Frame::hide_cursor()` exist (`include/koto/frame.hpp`). The real methods are `Frame(Terminal&)`, `Frame(int, int)`, `Frame::clear_back_buffer()`, `Frame::hit_at()`, and `std::optional<Area> Frame::bounds_of()`. |
| `docs/api/layout.md:141` | Claims `LayoutResult` implements `std::tuple_size` and `std::tuple_element`. | `LayoutResult` does not implement tuple protocol hooks (`include/koto/layout.hpp:109-183`); structured bindings work via the `Layout::vertical<N>` and `Layout::horizontal<N>` template overloads returning `std::array<Area, N>`. |
| `docs/api/terminal.md:38, 45-46, 56, 59` | Documents `restore_terminal() -> std::expected<...>`, `enable_mouse_tracking()`, `set_color_mode()`, and `Terminal::size()`. | `restore_terminal()` returns `void`; the mouse methods are `enable_mouse_capture()`, `enable_mouse_drag()`, and `enable_mouse_hover()`; the color mode setter is `color_mode(ColorMode)`; and `Terminal::size()` does not exist (`include/koto/terminal.hpp:54-81`). |
| `docs/architecture/event-loop.md:41, 114-118`, `docs/index.md:105, 108` | Refers to `Frame::render_diff()`, `KeyEvent{.key = Key::Char, .modifiers = Ctrl}`, and claims `Frame` swaps front and back buffer pointers. | The method is `Frame::render(bool flush = true)`; `KeyEvent` has `ch`, `ctrl`, `shift`, `alt`, `meta`; and `Frame::render()` copies dirty cells into `m_front_buffer` rather than swapping buffer pointers (`src/frame.cpp:405-409`). |
| `docs/architecture/focus-system.md:67, 79-85`, `docs/architecture/rendering-pipeline.md:20` | Refers to `frame.tag_hit_cell(...)` and shows an `IdScope` modal example with `Button().id("btn_ok").label("OK").draw(...)`. | The method is `frame.tag_cell(...)`; `.id("btn_ok")` is ambiguous without `Id("btn_ok")`; `Button` without `.focus(fm)` and `.on_click(...)` does not register in `FocusManager`; and a stack `IdScope` inside `on_render` pops at the end of `on_render` before input dispatch runs. |
| `docs/etc/performance-guide.md:18-22, 87-94` | Lists latency benchmarks (`~80 ns`, `~2.5 ns`, `~45 ns`, `~12 us`, `~1.1 ms`). | No benchmark program exists in the repository; all unverified benchmark numbers were removed. |
| `docs/etc/troubleshooting.md:29, 125-128` | Uses `Color::from_rgb(...)`, `KeyModifiers::Shift`, and claims `libkoto` decodes key release events and modifier-only keypresses. | `Color::from_rgb` and `KeyModifiers` do not exist, and `src/event.cpp:191-194, 297-300` explicitly ignores Kitty key release events (`event_type == 3`) and standalone modifier/lock keys. |
| `docs/getting-started.md:134, 153, 166, 208` | Calls `Box().border_fg(...)` and writes `std::expected<void, SystemError>`. | `Box::border_fg` and `SystemError` do not exist (`Box` uses `.fg(...)` and `.focused_border_fg(...)`, and `App::run()` returns `std::expected<void, TerminalErrorInfo>`). |
| `docs/widgets/box.md:82, 95, 154-155` | Claims default `BorderStyle::Rounded` and `TitleAlign::Left`, and claims `Box` truncates long titles. | `Box` defaults to `BorderStyle::Single` and `TitleAlign::Center` (`include/koto/box.hpp:59, 62`), and `src/box.cpp:268` omits the title if `text_width(m_title) + 4 > area.width`. |
| `docs/widgets/button.md:67-73` | Claims defaults `BorderStyle::Single`, `fg = COLOR_DEFAULT`, `label_fg = Color::White`, `focused_label_fg = Color::Yellow`. | Actual defaults in `include/koto/button.hpp:50-67` are `BorderStyle::Rounded`, `m_fg = Color::DarkGray`, `m_label_fg = Color::LightGray`, and `m_focused_label_fg = Color::White`. |
| `docs/widgets/list.md:91, 100-104` | Claims defaults `BorderStyle::Single`, `highlight_symbol = "> "`, `highlight_fg = Color::Yellow`, `overflow = TextOverflow::Clip`, and claims mouse wheel only scrolls viewport. | Actual defaults in `include/koto/list.hpp:78-86` are `BorderStyle::None`, `m_highlight_symbol = ""`, `m_highlighted_fg = COLOR_DEFAULT`, and `TextOverflow::Ellipsis`; mouse wheel calls `select_prev()` / `select_next()` (`src/list.cpp:159-168`). |
| `docs/widgets/tabs.md:97-98` | Claims defaults `active_fg = Color::Cyan`, `active_bg = COLOR_DEFAULT`, `inactive_fg = Color::DarkGray`. | Actual defaults in `include/koto/tabs.hpp:91-93` are `m_active_fg = Color::Black`, `m_active_bg = Color::Cyan`, and `m_inactive_fg = Color::White`. |
| `docs/widgets/text-area.md:111` | Claims `scroll_mode` defaults to `ScrollMode::Clamp`. | Actual default in `include/koto/text_area.hpp:39` is `ScrollMode::Margin` with `scroll_margin = 1`. |
| `docs/widgets/text-input.md:89, 104` | Claims `Alt+Backspace` triggers `delete_word()` and `placeholder_fg` defaults to `Color::DarkGray`. | `src/text_input.cpp:218` only binds `Ctrl+W` for `delete_word()` (and `Ctrl+U` for `clear()` at line 209); `m_placeholder_fg` defaults to `0x00888888` (`include/koto/text_input.hpp:79`). |
