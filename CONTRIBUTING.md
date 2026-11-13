# Contributing to koto

Thank you for your interest in contributing to **koto**!

koto is a zero-dependency, header-clean C++23 Terminal User Interface (TUI) library engineered for predictable latency, type-safe immediate-mode rendering, and high developer ergonomics.

To maintain production stability, koto follows a **Dual-Branch Workflow** (`main` and `dev`). All development occurs on topic branches targeting `dev`, while `main` is strictly reserved for stable, tagged releases.

---

## 1. Branching Architecture

```
[main]  ───● (v1.0.0) ──────────────────────────────● (v1.1.0 Release PR) ───> Production
              \                                    /
[dev]   ───────●────────●─────────●───────────────●───────────────────────────> Active Development
                        \        /
[fork]                   ●──────●  (feature/my-new-widget PR to dev)
```

| Branch | Purpose | Direct Pushes? | Pull Request Target |
| :--- | :--- | :--- | :--- |
| **`main`** | **Production & Releases**. Reflects tagged versions (`v1.0.0`, `v1.1.0`). Always deployable and tested. | ❌ Forbidden | Only merged via Release PRs from `dev` or critical hotfixes. |
| **`dev`** | **Active Integration**. All new features, performance enhancements, and bug fixes land here first. | ❌ Protected | **All contributor PRs MUST target `dev`**. |
| **`feature/*`**<br>**`fix/*`** | **Topic Branches**. Short-lived branches created on your personal fork, branched off latest `upstream/dev`. | ✅ On your fork | Submit PR targeting `base: dev`. |

---

## 2. Contributor Workflow (Step-by-Step)

### Step 1: Fork & Clone
1. Fork the official repository [`saber-88/koto`](https://github.com/saber-88/koto) to your GitHub account.
2. Clone your fork locally:
   ```bash
   git clone git@github.com:<your-username>/koto.git
   cd koto
   ```
3. Add the official upstream repository as a remote:
   ```bash
   git remote add upstream https://github.com/saber-88/koto.git
   git fetch upstream
   ```

### Step 2: Create a Feature Branch from `upstream/dev`
Always ensure your work branches off the freshest state of `upstream/dev`:
```bash
git fetch upstream dev
git checkout -b feature/my-cool-widget upstream/dev
```

> [!IMPORTANT]
> Never branch off `main` for new features or bug fixes. Always branch off `upstream/dev`.

### Step 3: Implement Your Changes
Follow koto's core design tenets:
- **C++23 Standards**: Use standard C++23 constructs (`std::string_view`, `std::expected`, `std::span`, concepts, structured bindings).
- **Zero Allocations in Hot Paths**: `on_render` callbacks and `Widget::draw` methods must avoid dynamic heap allocations (`std::string`, `new`, `std::vector::push_back`). Prefer string views, stack buffers, or pre-allocated states.
- **Header Purity**: Do not expose internal POSIX headers (`<termios.h>`, `<sys/ioctl.h>`) in `include/koto/`. Keep platform details encapsulated in `src/`.

### Step 4: Verify Locally (Zero Warnings & 100% Sanitizer Clean)
Before committing, your code must compile cleanly with **zero warnings** and pass all tests under both Release and Sanitizer-instrumented Debug builds:

```bash
# 1. Release build (Strict warnings & optimizations)
cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja
ninja -C build
ctest --test-dir build --output-on-failure

# 2. Debug build (AddressSanitizer + UndefinedBehaviorSanitizer)
cmake -B build_debug -DCMAKE_BUILD_TYPE=Debug -G Ninja
ninja -C build_debug
ctest --test-dir build_debug --output-on-failure
```

If you modify documentation under `docs/`:
```bash
cd docs
npm ci
npm run build
python3 tools/check-links.py dist
cd ..
```

### Step 5: Commit with Conventional Commits
Write clear, structured commit messages following [Conventional Commits](https://www.conventionalcommits.org/):
- `feat: add support for custom border characters in Box widget`
- `fix: correct mouse hit offset calculation in List scrolling`
- `docs: update focus management guide with modal trap example`
- `perf: eliminate string allocation in TextArea line number rendering`
- `test: add headless regression test for tab cycling boundary`

### Step 6: Push & Open a Pull Request
1. Push your topic branch to your fork:
   ```bash
   git push -u origin feature/my-cool-widget
   ```
2. Navigate to [`saber-88/koto`](https://github.com/saber-88/koto) on GitHub.
3. Click **Compare & pull request**.
4. **CRITICAL**: Set the base branch to **`dev`**:
   - `base: dev` ⬅ `compare: feature/my-cool-widget`
5. Fill out the pull request template checklist.
6. Ensure all automated GitHub Actions checks (GCC 13, Clang 17, ASan, UBSan, docs check) pass.

---

## 3. How Production Projects Manage the `dev` ➔ `main` Promotion

For project maintainers, here is the release lifecycle used by production-grade software:

### 1. Landing Contributions on `dev`
- Pull Requests are reviewed for API ergonomics, memory safety, and performance.
- When approved and CI is green, the PR is merged into `dev` via **Squash and Merge** (or Rebase Merge). This keeps `dev`'s commit history clean and linear.

### 2. Preparing a Release (`v1.x.y`)
When `dev` accumulates a milestone of features or scheduled updates:
1. Ensure all CI matrix jobs pass on `dev`.
2. Update the version string in `CMakeLists.txt` (and `docs/` if applicable).
3. Open a **Release PR** from `dev` into `main`:
   - Title: `release: v1.1.0`
   - Description: Detailed changelog of features, fixes, and breaking changes.
4. Merge `dev` into `main` using a **Standard Merge Commit** (`--no-ff`) to preserve branch topology.

### 3. Tagging & Publishing
```bash
git checkout main
git pull origin main
git tag -a v1.1.0 -m "Release v1.1.0: <summary of changes>"
git push origin v1.1.0
```
- GitHub Actions automatically triggers package builds (`CPack` `.tar.gz`, `.zip`) and updates the production documentation site.

### 4. Hotfix Protocol
If a critical security vulnerability or regression is discovered in the live `main` release:
1. Create a hotfix branch off `main`:
   ```bash
   git checkout -b hotfix/critical-fix main
   ```
2. Apply the fix, verify all tests, and merge into `main`.
3. Tag the patch release (e.g. `v1.0.1`).
4. **Backport to `dev` immediately**:
   ```bash
   git checkout dev
   git merge main
   git push origin dev
   ```
   This ensures `dev` never regresses on fixes already released in production.

---

## 4. Recommended GitHub Repository Settings

To enforce this workflow automatically on GitHub:

1. **Protect `main` Branch**:
   - Go to **Settings** ➔ **Branches** ➔ **Add branch protection rule** for `main`.
   - Check **Require a pull request before merging**.
   - Check **Require status checks to pass before merging** (select `build-and-test (GCC 13)`, `build-and-test (Clang 17)`, `docs-check`).
   - Check **Do not allow bypassing the above settings**.
   - Check **Require linear history** (or enforce squash/merge).
   - Check **Do not allow force pushes**.

2. **Protect `dev` Branch**:
   - Add branch protection rule for `dev`.
   - Check **Require a pull request before merging**.
   - Check **Require status checks to pass before merging**.
   - Check **Do not allow force pushes**.

3. **Default Branch Choice**:
   - Keep `main` as the default branch if you want GitHub visitors and `git clone` to land on the latest stable tagged code. The PR template and documentation explicitly instruct contributors to select `dev`.
   - *Alternative*: Set `dev` as the default branch on GitHub if you want every new pull request to automatically select `dev` as the base branch without manual dropdown selection.
