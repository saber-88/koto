## Description
<!-- Provide a clear, concise summary of your changes and why they are needed. -->

## Target Branch
- [ ] This PR targets the `dev` branch (PRs targeting `main` will be rejected unless this is an official release promotion).

## Type of Change
- [ ] `feat`: New widget, layout capability, or API feature
- [ ] `fix`: Bug fix in rendering, focus, input handling, or layout
- [ ] `perf`: Performance optimization (allocations, diffing, memory)
- [ ] `docs`: Documentation updates or corrections
- [ ] `refactor`: Internal code improvement with no API change
- [ ] `test`: New headless test suite or regression test

## Pre-Submission Verification Checklist
Please verify the following before submitting:
- [ ] Built and verified locally with zero compiler warnings under strict flags:
  ```bash
  cmake --preset release && cmake --build --preset release
  ```
- [ ] Verified 100% clean under AddressSanitizer (ASan) & UndefinedBehaviorSanitizer (UBSan):
  ```bash
  cmake --preset debug && ctest --preset debug --output-on-failure
  ```
- [ ] No heap allocations added inside immediate-mode render loops (`on_render`, `Widget::draw`).
- [ ] If adding new widgets or state properties, added headless unit tests under `tests/`.
- [ ] If modifying user-facing APIs, updated documentation and verified with `npm run build` in `docs/`.
