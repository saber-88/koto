# The libkoto Rendering Pipeline

Deep architectural analysis of `libkoto`'s deterministic double-buffered diffing pipeline, wide character continuation protocol, 44-byte cache alignment, and synchronized DEC 2026 atomic frame presentation.

---

## Architecture & System Overview

Terminal emulators communicate over standard output via byte streams of ANSI escape sequences. Redrawing complete screens on every tick causes severe screen tearing, high CPU usage, and noticeable input latency.

`libkoto` uses a **differential double-buffered pipeline** with synchronized updates:

```mermaid
flowchart TD
    subgraph Staging["1. Staging Pass (Virtual Back Buffer)"]
        WidgetDraw["Widget::draw(Frame&, Area)"]
        BackBuf["Frame::m_back_buffer (Row-Major Cells)"]
        HitBuf["Frame::m_hit_buffer (32-bit Spatial Id Tokens)"]
        WidgetDraw -->|set_cell / operator[]| BackBuf
        WidgetDraw -->|tag_hit_cell| HitBuf
    end

    subgraph DiffPass["2. Differential Scan & State Compression"]
        FrontBuf["Frame::m_front_buffer (Current Screen State)"]
        Comparator{"Cell != Front Cell?"}
        Skip["Skip Clean Cell (Zero Output)"]
        ANSIBatch["ANSI SGR Compression & Run-Length Batcher"]

        BackBuf --> Comparator
        FrontBuf --> Comparator
        Comparator -- No --> Skip
        Comparator -- Yes --> ANSIBatch
    end

    subgraph HardwareFlush["3. Atomic Synchronized Flush"]
        DEC2026Start["\033[?2026h (Halt Terminal Rasterizer)"]
        ByteStream["Batched SGR & Cursor Jumps"]
        DEC2026End["\033[?2026l (Atomic GPU Buffer Flip)"]
        TTY["Terminal TTY (Single write syscall)"]

        ANSIBatch --> DEC2026Start
        DEC2026Start --> ByteStream
        ByteStream --> DEC2026End
        DEC2026End --> TTY
    end
```

---

## Memory Layout & Cache Alignment

Every cell on the screen is represented by [`koto::Cell`](../api/cell.md):

```cpp
struct Cell
{
    Grapheme grapheme{};          // 24 bytes: inline UTF-8 cluster + metadata
    char32_t ch = U' ';           // 4 bytes: primary codepoint
    uint32_t fg = COLOR_DEFAULT;  // 4 bytes: 24-bit TrueColor foreground
    uint32_t bg = COLOR_DEFAULT;  // 4 bytes: 24-bit TrueColor background
    int width = 1;                // 4 bytes: display columns (0, 1, or 2)
    bool continuation = false;    // 1 byte: true if trailing cell of wide glyph
    uint8_t attrs = 0;            // 1 byte: bold, underline, italic, etc.
    // 2 bytes alignment padding
    // Total footprint: 44 bytes
};
```

### Hardware Cache Line Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ 64-Byte CPU L1 Cache Line                                                  │
├──────────────────────────────────────────────────────────────┬──────────────┤
│ Cell [row, col] (44 Bytes)                                   │ Next Cell... │
│ [Grapheme: 24B] [ch: 4B] [fg: 4B] [bg: 4B] [w: 4B] [c/a: 4B] │ (20 Bytes)   │
└──────────────────────────────────────────────────────────────┴──────────────┘
```

* **No Heap Indirection**: The 24-byte inline `Grapheme` buffer stores characters directly within the `Cell` struct. Rendering or diffing never dereferences external pointers.
* **Row-Major Memory Locality**: Buffers are indexed via $\text{Index} = \text{row} \times \text{width} + \text{col}$. Linear iteration sweeps cleanly through sequential L1 data cache lines.

---

## The Wide Character Continuation Protocol

CJK characters, emoji combinations, and full-width symbols occupy **two terminal columns**. If an engine tracks only the starting coordinate, subsequent overwrites produce corrupted character boundaries and cursor drift.

```
Grid Coordinates:       Col 0        Col 1        Col 2
Logical Characters:    [ '琴' (Width 2) ]        [ 'A' (Width 1) ]
Virtual Back Buffer:   [ Cell (Lead) ] [ Continuation ] [ Cell ('A') ]
ANSI Terminal Stream:  "琴A" (Continuation cell is skipped during diff emission)
```

### Overwrite & Boundary Invariants

1. **Automatic Companion Sentinel**:
   Writing a width-2 cell at `(row, col)` automatically places `Cell::make_continuation()` at `(row, col + 1)`.
2. **Left-Neighbor Invalidation**:
   Writing any character to `(row, col)` where `continuation == true` detects the collision and mutates `(row, col - 1)` into a clean single-column space (`U' '`).
3. **Right-Neighbor Invalidation**:
   Writing a width-1 character to `(row, col)` where a width-2 lead cell previously existed resets `(row, col + 1)` from continuation to a clean single-column space (`U' '`).
4. **Terminal Margin Protection**:
   A wide character written to the final column (`col == width - 1`) cannot fit its continuation cell and is automatically replaced with a single space `U' '`, preventing unintended terminal line wrapping.

---

## Synchronized Updates (DEC Mode 2026)

In GPU-accelerated terminal emulators (Kitty, Alacritty, Ghostty, WezTerm), frame drawing occurs concurrently with screen refresh. To prevent tearing artifacts:

1. Before emitting differential cells, `libkoto` outputs `\033[?2026h`. The terminal emulator pauses its display rendering loop.
2. The entire diff buffer is transmitted across the TTY in a single `write()` system call.
3. `libkoto` outputs `\033[?2026l`. The terminal emulator immediately flips the newly updated buffer onto the screen in a single GPU raster pass.

---

## C++ Language Features Utilized

1. **Multi-Dimensional Subscripting (`operator[r, c]`) (C++23)**: `frame[r, c]` provides clean, zero-overhead 2D array indexing without syntax hacks.
2. **`std::string_view` (C++17)**: `c.grapheme.view()` appends directly into output buffers without string copies or memory allocations.
3. **Cache-Aligned Aggregate Design (C++11)**: Packing `Cell` into 44 bytes aligns with hardware cache line boundaries for optimal memory prefetching.
4. **Move Semantics (`std::move`) (C++11)**: Buffer swaps transfer ownership of grid vectors without deep memory duplication.
5. **Static Factory Methods**: `Cell::make_continuation()` provides an explicit, self-documenting construction idiom.
