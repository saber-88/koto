#!/usr/bin/env python3
"""
Documentation test tool for libkoto.

Every ```cpp block in src/content/docs is compiled against the real library, and
blocks that carry  render="WxH"  are executed headlessly: the widget code draws into
an in-memory koto::Frame and the plain-text result is written back into the page as a
following  ```text title="Output"  block. Docs examples can therefore never drift from
the library or show output the library does not actually produce.

    python3 tools/snippets.py check             compile every C++ block        (CI)
    python3 tools/snippets.py render            regenerate all Output blocks
    python3 tools/snippets.py render --verify   fail if any Output block is stale (CI)

Where the koto sources live:   --koto PATH   or   $KOTO_DIR   (default: the parent directory,
i.e. the layout where this site lives in <koto-repo>/docs/).

Block meta understood (after ```cpp):
    render="40x6"   run headlessly in a 40-column x 6-row Frame and embed the output
    skip            do not compile this block (pseudo-code, intentionally partial code)

Fragments (blocks without `int main`) are compiled inside a function that provides
    App& app, FocusManager& fm, Frame& frame
and any undeclared identifier ending in `_area` is provided as an Area. Complete programs
(blocks with `int main`) are compiled with the library's own strict warning flags.
"""
import argparse, concurrent.futures as cf, hashlib, os, re, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
DOCS = HERE.parent / "src" / "content" / "docs"
CACHE = HERE / ".cache"

FENCE = re.compile(r"^```(\w[\w+-]*)([^\n]*)\n(.*?)^```[ \t]*$", re.S | re.M)
STRICT = ["-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wsign-conversion",
          "-Wshadow", "-Wold-style-cast", "-Werror"]
PRELUDE_HEADER = """#pragma once
#include <koto/koto.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
using namespace koto;
"""
PRELUDE = '#include "snippet_prelude.hpp"\n'


def koto_dir(arg):
    p = Path(arg or os.environ.get("KOTO_DIR") or HERE.parent.parent).resolve()
    if not (p / "include" / "koto" / "koto.hpp").exists():
        sys.exit(f"error: cannot find koto sources at {p} (use --koto PATH or $KOTO_DIR)")
    return p

def cxx():
    return os.environ.get("CXX", "g++")

def build_lib(koto):
    """Compile the library once into tools/.cache/libkoto.a (rebuilt when sources change)."""
    CACHE.mkdir(exist_ok=True)
    srcs = sorted((koto / "src").glob("*.cpp"))
    stamp = hashlib.sha1(b"".join(s.read_bytes() for s in srcs) +
                         b"".join(h.read_bytes() for h in sorted((koto / "include/koto").glob("*.hpp")))).hexdigest()[:12]
    lib = CACHE / f"libkoto-{stamp}.a"
    if lib.exists():
        return lib
    for old in CACHE.glob("libkoto-*.a"):
        old.unlink()
    objs = []
    with tempfile.TemporaryDirectory() as td:
        def one(s):
            o = Path(td) / (s.stem + ".o")
            r = subprocess.run([cxx(), "-std=c++23", "-O1", f"-I{koto/'include'}", "-c", str(s), "-o", str(o)],
                               capture_output=True, text=True)
            if r.returncode:
                sys.exit(f"error: library failed to compile ({s.name}):\n{r.stderr[:2000]}")
            return o
        with cf.ThreadPoolExecutor(os.cpu_count() or 4) as ex:
            objs = list(ex.map(one, srcs))
        subprocess.run(["ar", "rcs", str(lib)] + [str(o) for o in objs], check=True)
    return lib

class Block:
    def __init__(self, path, line, lang, meta, code, start, end):
        self.path, self.line, self.lang, self.meta, self.code = path, line, lang, meta, code
        self.start, self.end = start, end          # span of the whole fenced block in the file text
        m = re.search(r'render="(\d+)x(\d+)"', meta)
        self.render = (int(m.group(1)), int(m.group(2))) if m else None
        self.skip = bool(re.search(r"(^|\s)skip(\s|$)", meta))
        code_no_str = re.sub(r'"(\\.|[^"\\])*"', '""', re.sub(r'//.*', '', code))
        self.is_main = bool(re.search(r"\bint\s+main\s*\(", code_no_str))
    @property
    def label(self):
        return f"{self.path.relative_to(DOCS)}:{self.line}"

def blocks_in(path):
    text = path.read_text()
    out = []
    for m in FENCE.finditer(text):
        out.append(Block(path, text.count("\n", 0, m.start()) + 1, m.group(1), m.group(2), m.group(3), m.start(), m.end()))
    return text, out

def all_cpp():
    res = []
    for p in sorted(DOCS.rglob("*.md*")):
        _, bl = blocks_in(p)
        res += [b for b in bl if b.lang in ("cpp", "c++") and not b.skip]
    return res

def area_decls(code, init):
    names = sorted(set(re.findall(r"\b([a-z][a-z0-9_]*_area)\b", code)))
    decl = []
    for n in names:
        if re.search(rf"\b(Area|auto)\s+{n}\b", code):
            continue
        decl.append(f"Area {n} = {init};")
    return "\n    ".join(decl)

_pch_ready = False
def ensure_pch(koto):
    """Precompile the common prelude (koto.hpp + std headers) once; makes the check ~10x faster."""
    global _pch_ready
    if _pch_ready:
        return
    CACHE.mkdir(exist_ok=True)
    hdr = CACHE / "snippet_prelude.hpp"
    stamp = hashlib.sha1(PRELUDE_HEADER.encode() + b"".join(h.read_bytes() for h in sorted((koto / "include/koto").glob("*.hpp")))).hexdigest()[:12]
    marker = CACHE / f"pch-{stamp}"
    if not marker.exists():
        for old in CACHE.glob("pch-*"):
            old.unlink()
        hdr.write_text(PRELUDE_HEADER)
        r = subprocess.run([cxx(), "-std=c++23", "-w", f"-I{koto/'include'}", "-x", "c++-header", str(hdr), "-o", str(hdr) + ".gch"],
                           capture_output=True, text=True)
        if r.returncode:
            sys.exit("error: could not precompile prelude:\n" + r.stderr[:2000])
        marker.write_text("ok")
    _pch_ready = True

def compile_src(src, koto, extra=None, link=None, out=None):
    ensure_pch(koto)
    with tempfile.TemporaryDirectory() as td:
        f = Path(td) / "snippet.cpp"
        f.write_text(src)
        cmd = [cxx(), "-std=c++23", f"-I{koto/'include'}", f"-I{CACHE}"] + (extra or [])
        cmd += ["-o", out or str(Path(td) / "a.out")] if link else ["-fsyntax-only"]
        cmd += [str(f)] + ([str(link)] if link else [])
        r = subprocess.run(cmd, capture_output=True, text=True)
        return r.returncode == 0, r.stderr

def check_block(b, koto):
    code = b.code
    if b.is_main:
        src = code if "#include" in code else PRELUDE + code
        ok, err = compile_src(src, koto, STRICT)
        return b, ok, err
    # fragment: first try as namespace-scope declarations, then inside a function body
    ns_src = PRELUDE + code
    ok_ns, err_ns = compile_src(ns_src, koto, ["-w"])
    if ok_ns:
        return b, True, ""
    fn_src = (PRELUDE + "void snippet_ctx(App& app, FocusManager& fm, Frame& frame) {\n"
              "    (void)app; (void)fm; (void)frame;\n    " + area_decls(code, "frame.area()") + "\n    {\n" + code + "\n    }\n}\n")
    ok_fn, err_fn = compile_src(fn_src, koto, ["-w"])
    if ok_fn:
        return b, True, ""
    starts_decl = re.match(r"\s*(struct|class|enum|using|namespace|template|#include|inline|constexpr)\b", code)
    return b, False, err_ns if starts_decl else err_fn

def render_block(b, koto, lib):
    cols, rows = b.render
    src = (PRELUDE + "int main() {\n"
           f"    Frame frame({rows}, {cols});\n    FocusManager fm; (void)fm;\n    "
           + area_decls(b.code, "frame.area()") + "\n    {\n" + b.code + "\n    }\n"
           "    std::cout << frame.to_plain_text();\n    return 0;\n}\n")
    with tempfile.TemporaryDirectory() as td:
        exe = str(Path(td) / "run")
        ok, err = compile_src(src, koto, ["-w", "-O0"], link=lib, out=exe)
        if not ok:
            return b, None, err
        r = subprocess.run([exe], capture_output=True, text=True, timeout=20)
        if r.returncode:
            return b, None, f"exit {r.returncode}\n{r.stderr}"
        lines = [l.rstrip() for l in r.stdout.split("\n")]
        while lines and not lines[-1]:
            lines.pop()
        while lines and not lines[0]:
            lines.pop(0)
        return b, "\n".join(lines), ""

def cmd_check(args):
    koto = koto_dir(args.koto)
    blocks = all_cpp()
    with cf.ThreadPoolExecutor(os.cpu_count() or 4) as ex:
        results = list(ex.map(lambda b: check_block(b, koto), blocks))
    bad = [(b, e) for b, ok, e in results if not ok]
    for b, e in bad:
        print(f"\n✗ {b.label}")
        for l in [l for l in e.splitlines() if "error" in l][:4]:
            print("   ", re.sub(r"^.*snippet\.cpp:", "snippet:", l)[:220])
    print(f"\n{len(blocks) - len(bad)}/{len(blocks)} C++ blocks compile against koto")
    return 1 if bad else 0

OUT_RE = re.compile(r'\n*```text[^\n]*title="Output"[^\n]*\n.*?^```[ \t]*$', re.S | re.M)

def cmd_render(args):
    koto = koto_dir(args.koto)
    lib = build_lib(koto)
    stale, failed, written = [], [], 0
    for p in sorted(DOCS.rglob("*.md*")):
        text, bl = blocks_in(p)
        todo = [b for b in bl if b.lang in ("cpp", "c++") and b.render and not b.skip]
        if not todo:
            continue
        with cf.ThreadPoolExecutor(os.cpu_count() or 4) as ex:
            res = list(ex.map(lambda b: render_block(b, koto, lib), todo))
        # apply edits back-to-front so earlier offsets stay valid
        for b, out, err in sorted(res, key=lambda r: -r[0].start):
            if out is None:
                failed.append((b, err)); continue
            new_block = f'\n\n```text frame="terminal" title="Output"\n{out}\n```'
            tail = text[b.end:]
            m = OUT_RE.match(tail)
            old_end = b.end + (m.end() if m else 0)
            if m and text[b.end:old_end] == new_block:
                continue
            stale.append(b)
            text = text[:b.end] + new_block + text[old_end:]
        if not args.verify:
            p.write_text(text)
            written += 1
    for b, e in failed:
        print(f"✗ {b.label} failed to render:\n" + "\n".join(l for l in e.splitlines() if "error" in l)[:600])
    if args.verify:
        for b in stale:
            print(f"✗ stale output: {b.label}")
        print(f"{len(stale)} stale, {len(failed)} failed")
        return 1 if (stale or failed) else 0
    print(f"rendered {sum(1 for p in DOCS.rglob('*.md*') for b in blocks_in(p)[1] if b.render)} outputs ({len(stale)} changed, {len(failed)} failed)")
    return 1 if failed else 0

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mode", choices=["check", "render"])
    ap.add_argument("--koto"); ap.add_argument("--verify", action="store_true")
    a = ap.parse_args()
    sys.exit(cmd_check(a) if a.mode == "check" else cmd_render(a))

if __name__ == "__main__":
    main()
