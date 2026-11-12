#!/usr/bin/env python3
"""Check internal links and assets in the built site (dist/). Run after `npm run build`.

    python3 tools/check-links.py [dist-dir] [--base /koto]

Fails if any href/src that points inside the site does not resolve to a file, or if a
#fragment does not match an id on the target page.
"""
import re, sys
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urldefrag, urlparse, unquote

args = [a for a in sys.argv[1:] if not a.startswith("--")]
dist = Path(args[0] if args else "dist").resolve()
import os
base = os.environ.get("BASE_PATH", "/")
if not base.endswith("/"): base += "/"
if "--base" in sys.argv:
    base = sys.argv[sys.argv.index("--base") + 1].rstrip("/") + "/"

class P(HTMLParser):
    def __init__(self):
        super().__init__(); self.links, self.ids = [], set()
    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        if "id" in a: self.ids.add(a["id"])
        if tag == "a" and a.get("href"): self.links.append(a["href"])
        if tag in ("img", "script", "source") and a.get("src"): self.links.append(a["src"])
        if tag == "link" and a.get("href") and a.get("rel") in ("stylesheet", "icon", "apple-touch-icon"): self.links.append(a["href"])

pages = {}
for f in dist.rglob("*.html"):
    p = P(); p.feed(f.read_text(errors="ignore")); pages[f] = p

def resolve(src_file, href):
    href, frag = urldefrag(href)
    u = urlparse(href)
    if u.scheme or u.netloc or href.startswith(("mailto:", "tel:", "data:")):
        return "external", None
    path = unquote(u.path)
    if not path:
        return "ok", src_file if frag else None
    if not path.startswith("/"):
        return "relative", None
    if path.startswith("/"):
        if base != "/" and path.startswith(base): path = "/" + path[len(base):]
        elif base != "/":
            # a root-absolute link that does not start with the base: broken if it names a page of this site
            guess = dist / path.lstrip("/")
            if any(c.is_file() for c in (guess, guess / "index.html", Path(str(guess) + ".html"))):
                return "missing", guess
            return "ok", None
        target = dist / path.lstrip("/")
    else:
        target = (src_file.parent / path).resolve()
    for cand in (target, target / "index.html", Path(str(target) + ".html")):
        if cand.is_file(): return "ok", cand
    return "missing", target

bad = []
for f, p in pages.items():
    for href in p.links:
        status, target = resolve(f, href)
        if status == "relative":
            bad.append((f.relative_to(dist), href, "relative link: use a root-absolute path like /guides/colors/"))
        elif status == "missing":
            bad.append((f.relative_to(dist), href, "no such file"))
        elif status == "ok" and "#" in href and target and target in pages:
            frag = urldefrag(href)[1]
            if frag and frag not in pages[target].ids:
                bad.append((f.relative_to(dist), href, "no such anchor"))
for page, href, why in sorted(set(bad)):
    print(f"✗ {page}: {href} ({why})")
print(f"checked {sum(len(p.links) for p in pages.values())} links in {len(pages)} pages, {len(set(bad))} broken")
sys.exit(1 if bad else 0)
