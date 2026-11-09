# `koto::App`

Top-level application orchestrator responsible for terminal lifecycle, event dispatch, frame timing (~60 FPS), background worker synchronization, and double-buffered render passes.

---

## Header & Class Synopsis

```cpp
#include <koto/app.hpp>
```

```cpp
namespace koto
{
    class App
    {
    public:
        App();
        ~App();

        // Lifecycle & Loop
        [[nodiscard]] auto run() -> std::expected<void, TerminalErrorInfo>;
        [[nodiscard]] auto run(std::function<void(Frame&)> render_fn) -> std::expected<void, TerminalErrorInfo>;
        void quit();
        void wakeup();

        // Event & Hook Registration
        void on_key(std::function<void(const KeyEvent&)> handler);
        void on_mouse(std::function<void(const MouseEvent&)> handler);
        void on_resize(std::function<void(const ResizeEvent&)> handler);
        void on_paste(std::function<void(const PasteEvent&)> handler);
        void on_render(std::function<void(Frame&)> handler);

        // Periodic Animation Ticks (~60 FPS)
        void on_tick(std::function<void()> handler);
        void on_tick(std::function<void(double dt)> handler);

        // Integrated Focus Management
        void use_focus(FocusManager& fm);

        // Geometry Accessors
        [[nodiscard]] Area area() const noexcept;
        [[nodiscard]] int rows() const noexcept;
        [[nodiscard]] int cols() const noexcept;
    };
}
```

---

## Member Functions Reference

| Method | Signature | Description |
|:---|:---|:---|
| **`run()`** | `auto run() -> std::expected<void, TerminalErrorInfo>` | Enters terminal raw mode, sets up signal handlers, and executes the 60 FPS event loop. |
| **`quit()`** | `void quit()` | Flags the event loop to stop, cleanly restoring the cooked terminal upon exit. |
| **`wakeup()`**| `void wakeup()` | Thread-safe, non-blocking asynchronous wakeup via POSIX self-pipe. |
| **`on_key()`** | `void on_key(std::function<void(const KeyEvent&)>)` | Subscribes to keyboard events (ASCII, escape keys, Kitty CSI u). |
| **`on_mouse()`** | `void on_mouse(std::function<void(const MouseEvent&)>)` | Subscribes to mouse clicks, moves, and wheel scrolls. |
| **`on_resize()`**| `void on_resize(std::function<void(const ResizeEvent&)>)` | Invoked when the terminal window dimensions change (`SIGWINCH`). |
| **`on_paste()`** | `void on_paste(std::function<void(const PasteEvent&)>)` | Receives clipboard paste streams in bracketed paste mode. |
| **`on_render()`**| `void on_render(std::function<void(Frame&)>)` | Drawing callback invoked whenever the screen requires redrawing. |
| **`on_tick()`** | `void on_tick(std::function<void(double dt)>)` | Animation tick handler executing at ~60 FPS with delta time in seconds. |
| **`use_focus()`**| `void use_focus(FocusManager& fm)` | Routes keyboard and mouse events to focused widgets before invoking user hooks. |
| **`area()`** | `Area area() const noexcept` | Returns the current full screen bounding rectangle `(row: 0, col: 0, width, height)`. |

---

## Usage Examples

### 1. Basic Interactive App

```cpp
#include <koto/koto.hpp>

using namespace koto;

int main()
{
    App app;

    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'q') {
            app.quit();
        }
    });

    app.on_render([&](Frame& frame) {
        Box().title("Hello").draw(frame, frame.area());
        Text("Press 'q' to quit.").draw(frame, frame.area().inset(1, 1, 1, 1));
    });

    auto res = app.run();
    if (!res) {
        std::fprintf(stderr, "Error: %.*s\n",
                     static_cast<int>(res.error().message().size()),
                     res.error().message().data());
        return 1;
    }
    return 0;
}
```

### 2. Multi-Threaded Telemetry with `app.wakeup()`

```cpp
#include <koto/koto.hpp>
#include <thread>
#include <atomic>

using namespace koto;

int main()
{
    App app;
    std::atomic<int> shared_data = 0;
    std::atomic<bool> running = true;

    // Background worker thread
    std::thread worker([&]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            shared_data.fetch_add(1);
            app.wakeup(); // Wakes event loop asynchronously via POSIX self-pipe
        }
    });

    app.on_key([&](const KeyEvent& k) {
        if (k.ch == 'q') app.quit();
    });

    app.on_render([&](Frame& frame) {
        std::string label = std::format("Telemetry Counter: {}", shared_data.load());
        Text(label).draw(frame, frame.area());
    });

    app.run();

    running = false;
    worker.join();
    return 0;
}
```

---

## Invariants & Safety Guarantees

1. **RAII Teardown Guarantee**: Terminal raw mode and mouse tracking flags are always restored when `run()` returns, even during exceptions or early returns.
2. **Zero-Overhead Idle Sleep**: When no `on_tick` animation handler is registered, `App` blocks inside `poll(-1)`, consuming exactly 0% CPU.
3. **Burst Event Coalescing**: Input bursts are fully drained from standard input before triggering a redraw pass, avoiding multi-draw stuttering.

---

## C++ Language Features Utilized

1. **`std::expected<void, TerminalErrorInfo>` (C++23)**: Monadic error handling providing descriptive POSIX initialization failure diagnostics without throwing exceptions.
2. **`[[nodiscard]]` (C++17)**: Prevents silently ignoring error return values from `run()`.
3. **`noexcept` Accessors (C++11)**: `area()`, `rows()`, and `cols()` guarantee no-throw geometry queries.
4. **Move-Only Functors & Reference Captures**: Flexible, zero-overhead lambda callback integration.
