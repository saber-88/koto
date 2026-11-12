# libkoto documentation

Astro Starlight site for [libkoto](https://github.com/OWNER/koto). Monochrome, light and dark, no client-side framework.

```bash
npm ci
npm run dev          # http://localhost:4321/koto/
npm run build        # static site in dist/
```

## Design

The look follows the zkPass style reference: near-black canvas, a single signal-lime accent (`#c5ff4a`, olive `#597321` for text in light mode), Source Serif 4 at weight 300 for display type with one italic lime word per headline, Inter Tight for UI, JetBrains Mono NL for code, `[ LABEL ]` metadata stamps, sharp corners and hairline borders. Fonts are self-hosted (`@fontsource-variable/*`, plus a subset of JetBrains Mono in `src/assets/fonts`, OFL 1.1). The grid backdrop and cursor light live in `public/koto-fx.js` and the end of `src/styles/theme.css`. Tokens are CSS variables at the top of `theme.css`.

After changing the theme, delete `.astro/` and `dist/` before rebuilding. A stale cache can leave pages pointing at a stylesheet hash that no longer exists.

## The docs are tested

Every C++ code block under `src/content/docs` is compiled against the library, and blocks marked `render="WxH"` are executed headlessly so their output is generated, not typed.

```bash
python3 tools/snippets.py check             # compile every cpp block
python3 tools/snippets.py render            # regenerate all "Output" blocks in place
python3 tools/snippets.py render --verify   # fail if any output block is stale
python3 tools/check-links.py dist --base /koto   # after `npm run build`
npm run check                                # all of the above
```

The tool finds the library at `../` (this site lives in `<repo>/docs`) or at `--koto PATH` / `$KOTO_DIR`. Set `CXX` to pick a compiler (default `g++`).

Fence meta understood after the language: `render="40x6"` (run in a 40-column by 6-row frame and embed output) and `skip` (do not compile; use sparingly).

A cpp block without `int main` is compiled inside a function that provides `App& app`, `FocusManager& fm`, `Frame& frame`, and any undeclared `*_area` name as an `Area`. A block with `int main` is a complete program and is compiled with strict warnings (`-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`).

## Adding a page

1. Create `src/content/docs/<folder>/<name>.md` with this frontmatter (the only required key is `title`):

   ```md
   ---
   title: My page
   description: One sentence for search results and link previews.
   sidebar:
     label: Short name        # optional: text shown in the sidebar
     order: 4                 # optional: position inside an auto-generated group
   ---
   ```

2. Show it in the sidebar. `architecture/` and `deep-dives/` are auto-generated, so a file dropped there appears on its own. Every other group is listed by hand in `astro.config.mjs`, so add `{ slug: 'widgets/my-widget', label: 'MyWidget' }` to the right `items` array. The slug is the path under `src/content/docs/` without `.md`.
3. Link to other pages with root-absolute paths and no base: `[List](/widgets/list/)`. The remark plugin adds `/koto`.
4. In code fences, ` ```cpp ` blocks are compiled against the library by `npm run check`. Add `render="40x6"` to also run it and embed the real output, or `skip` for pseudo-code. ` ```mermaid ` fences become diagrams.
5. Check it: `npm run dev`, then `python3 tools/snippets.py check`.

To bring in a plain Markdown file that has no frontmatter yet:

```bash
python3 tools/import-md.py ../docs/etc/faq.md deep-dives/faq --label FAQ --order 4
```

It adds the frontmatter, drops the leading `# Heading`, fixes relative links, and leaves the rest untouched.

## Configuration

`src/config.ts` holds the project name, repository URL (default `https://github.com/saber-88/koto`), site URL and base path; each can be overridden with `REPO_URL`, `SITE_URL`, `BASE_PATH`, `EDIT_URL`. Docs pages write links as `github.com/OWNER/koto`, and the remark plugin rewrites them to the configured repository. Deployment: `.github/workflows/deploy.yml` (GitHub Pages) or `netlify.toml`.
