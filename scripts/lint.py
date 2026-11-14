#!/usr/bin/env python3
"""Runs clang-tidy and clang-format on src/ and writes reports/lint.json.

Usage: lint.py --build-dir build --output reports/lint.json

Output format (reports/lint.json):
  {"tool": "clang-tidy", "errors": [{"path": "src/...", "line": 42, "code": "...", "message": "..."}]}

clang-format violations are reported with code "clang-format". Exit code is 0 whenever
the linters ran, whatever they found; 2 if a linter is missing or crashed.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

DIAG = re.compile(r"^(?P<path>[^:\n]+):(?P<line>\d+):(?P<col>\d+): (?P<kind>warning|error): "
                  r"(?P<message>.*?)(?: \[(?P<code>[^\]]+)\])?$")


def find_tool(name: str) -> str | None:
    env = os.environ.get(name.upper().replace("-", "_"))
    if env:
        return env
    found = shutil.which(name)
    if found:
        return found
    for version in range(25, 13, -1):
        found = shutil.which(f"{name}-{version}")
        if found:
            return found
    return None


def relative(path: str, root: Path) -> str:
    try:
        return str(Path(path).resolve().relative_to(root))
    except ValueError:
        return path


def parse(output: str, root: Path, default_code: str) -> list[dict]:
    errors = []
    for line in output.splitlines():
        match = DIAG.match(line.strip())
        if not match:
            continue
        rel = relative(match["path"], root)
        if not rel.startswith("src/"):
            continue
        errors.append({"path": rel, "line": int(match["line"]),
                       "code": match["code"] or default_code, "message": match["message"]})
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--output", default="reports/lint.json")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 2)
    args = parser.parse_args()

    root = Path.cwd().resolve()
    tidy = find_tool("clang-tidy")
    fmt = find_tool("clang-format")
    if tidy is None or fmt is None:
        print("lint: clang-tidy and clang-format are required", file=sys.stderr)
        return 2
    compile_db = Path(args.build_dir) / "compile_commands.json"
    if not compile_db.exists():
        print(f"lint: {compile_db} missing; configure the build first", file=sys.stderr)
        return 2

    sources = sorted(str(p) for p in Path("src").rglob("*.cpp"))
    headers = sorted(str(p) for p in Path("src").rglob("*.hpp"))

    def run_tidy(source: str) -> tuple[int, str]:
        proc = subprocess.run([tidy, "--quiet", "-p", args.build_dir, source],
                              capture_output=True, text=True, errors="replace")
        return proc.returncode, proc.stdout

    errors: list[dict] = []
    with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        results = list(pool.map(run_tidy, sources))
    for code, out in results:
        if code not in (0, 1):
            print(f"lint: clang-tidy crashed (exit {code})", file=sys.stderr)
            return 2
        errors.extend(parse(out, root, "clang-tidy"))

    proc = subprocess.run([fmt, "--dry-run", "--style=file", *sources, *headers],
                          capture_output=True, text=True, errors="replace")
    if proc.returncode not in (0, 1):
        print(f"lint: clang-format crashed (exit {proc.returncode})", file=sys.stderr)
        return 2
    errors.extend(parse(proc.stderr, root, "clang-format"))

    # Headers are analysed once per including TU; report each diagnostic once.
    unique = {(e["path"], e["line"], e["code"], e["message"]): e for e in errors}
    errors = sorted(unique.values(), key=lambda e: (e["path"], e["line"], e["code"]))
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    Path(args.output).write_text(json.dumps({"tool": "clang-tidy", "errors": errors}, indent=2) + "\n")
    print(f"lint: {len(errors)} issue(s) written to {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
