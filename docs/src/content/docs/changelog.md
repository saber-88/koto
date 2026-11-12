---
title: Changelog
description: Commit history of libkoto recorded from git log up to commit 51c6c1dc7d25cb805ca620ef9b8af699941e14f1
sidebar:
  label: Changelog
  order: 99
---

## Version 1.0.0 (`PROJECT_VERSION` in `CMakeLists.txt:2`)

All entries below are recorded directly from the repository `git log` up to `HEAD` (`51c6c1dc7d25cb805ca620ef9b8af699941e14f1`).

<!-- src: CMakeLists.txt:2 -->

| Commit | Summary |
| :--- | :--- |
| `51c6c1d` | `ci: rely on CMakeLists.txt for Clang 17 concepts workaround` |
| `d85b6d4` | `perf: pack Cell layout to 64-byte cache line and eliminate draw-call string allocations` |
| `389b62c` | `fix(ci): work around Clang 17 __cpp_concepts macro deficit for libstdc++ std::expected` |
| `a6c3d49` | `fix: resolve third-pass audit items (N1-N4), CI toolchain, and ABI stability` |
| `9cf6caf` | `updated README.md` |
| `520626e` | `fix & feat: resolve all review findings (C1-C2, H1-H3, M1-M6, R1-R8) & expand test suites` |
| `d2786f1` | `docs: update headless tests badge to 32 passing` |
| `7c98289` | `fix: resolve library review findings (layout array overload, button id ambiguity, word wrap, emoji ZWJ, modal target pruning, cxx_std_23 export)` |
| `e016a77` | `docs & ci: update README to 31 tests & add GitHub Actions CI workflow` |
| `039b2a1` | `refactor: unify Color value type, geometry accessors, and focus API` |
| `19bb0a3` | `feat: resolve audit concerns C-01 to C-36 and add zero-allocation layout engine` |
| `1cd1db0` | `refactor(event): implement cross-platform POSIX self-pipe trick and async wakeup` |
| `714f208` | `fix(app): drain and batch queued input events before rendering to eliminate input lag propagation` |
| `23d1b0a` | `feat(demos): add 3D window demoparty showcase with subpixel half-blocks and 4 real-time scenes` |
| `0fa8596` | `feat(examples): add 3D Saturn simulation and Full Solar System interactive animation` |
| `27659e4` | `fix(button): properly propagate focused_fg to label and border styling` |
| `877afe7` | `fix(utf8): eliminate -Wsign-conversion warnings in next_codepoint bitwise decoding` |
| `da48b62` | `fix(terminal): add write_all loop for EINTR/short-writes and SIGTERM/SIGHUP emergency recovery` |
| `e66fd25` | `feat(focus): add FocusManager, 2D spatial navigation, hit-buffer mouse engine & modal trapping` |
| `5762b23` | `feat(app): implement App::use_focus, SGR mouse tracking, and automated focus wiring` |
| `ed16163` | `feat(focus): implement auto-harvested spatial 2D navigation and modal focus trapping` |
| `8587037` | `feat(focus): implement universal Id token and FocusManager` |
| `5785412` | `feat(focus): implement Frame spatial hit-buffer and Box visual focus` |
| `76630a6` | `style(core): clarify member access and invocations with explicit this->` |
| `a612421` | `refactor(core): modernize pointer usage with zero-overhead C++23 abstractions` |
| `15118f6` | `feat(core): adopt C++23 multidimensional operator[], std::expected, and std::views::enumerate` |
| `eceba36` | `refactor(event): modernize parse_csi and parse_ss3 to take std::string_view` |
| `24ff0d5` | `docs: replace raw LaTeX math syntax with clean markdown and Unicode in README` |
| `d180533` | `feat(core): implement DEC Mode 2026, Kitty CSI u keyboard protocol, and bracketed paste` |
| `f74d4db` | `Add MIT License to the project` |
| `2722f38` | `feat: add Grid layout, TextArea, Tabs, Gauge, Sparkline, and Rule widgets` |
| `f753aea` | `feat: complete production-ready Layer 1 & 2 core, App runtime, and headless test suite` |
| `56af8fb` | `implemented vbox and hbox widgets` |
| `e6d7207` | `implemented fundamnetal structure of lib` |
| `1b9fc02` | `recreated entire lib` |
| `5baeb73` | `initial commit` |

## Key architectural milestones in git history

- **64-byte `Cell` packing (`d85b6d4`)**: Expanded `Grapheme::data` to `char data[36]` (`44` bytes total for `Grapheme`) and added `uint8_t reserved[5]` to `Cell` with `static_assert(sizeof(Cell) == 64)`.
- **Third-pass audit & CI hardening (`a6c3d49`, `389b62c`, `51c6c1d`)**: Added pointer-collision tracking (`state_ptr` and `has_custom_id` on `FocusTarget`), host termination signal handler chaining (`N3`), 500 ms / 4 MB bracketed paste timeout (`N4`), and the Clang `< 19.0` `__cpp_concepts=202002L` CMake definition.
- **Second-pass review & headless suites (`520626e`, `7c98289`)**: Added `std::array<Constraint, N>` layout overloads, word-boundary wrapping in `Text::wrap()`, modal target pruning in `FocusManager::pop_scope()`, hidden-widget skipping in `FocusManager::next()`/`prev()`, and modularized the headless test suite into 6 CTest targets.
- **Zero-allocation `LayoutResult` & audit C-01..C-36 (`19bb0a3`, `039b2a1`)**: Introduced the 16-element stack-buffered `LayoutResult`, unified `Color` value conversions, and added SGR text attributes (`TextAttr`).
- **Focus system & spatial hit-buffer (`5785412`..`e66fd25`)**: Added `koto::Id`, `Frame` hit-buffer tagging and bounding-box harvesting, `FocusManager`, 2D spatial arrow navigation, and `IdScope` modal trapping.

## See also

- [System Requirements](/getting-started/requirements/)
- [Limitations and FAQ](/limitations-faq/)
- [App](/api/app/)
