#!/usr/bin/env python3
"""Generates .virtint/functions.json: every public ShopCore function with its source span
and the tests that pin it (consumed by the issue factory indexer).

Usage: gen_functions_json.py [--check]

Conventions it relies on (enforced by --check):
  * Public functions are declared in src/shopcore/<module>/<module>.hpp and defined at
    column 0 in src/shopcore/<module>/<file>.cpp, outside the anonymous namespace; the
    body ends at the next line that is exactly "}".
  * Tests for src/shopcore/<module>/<file>.cpp live in tests/<module>/<file>_test.cpp and
    are named TEST(Suite, <function>_<case>). A test belongs to the longest function name
    in that file that prefixes it.
  * core/ is shared infrastructure, not a learner-facing module, and is not listed.
With --check the script fails if any public function has fewer than 3 tests or any test
cannot be mapped to a function.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

MODULES = ["money", "catalog", "inventory", "pricing", "tax", "cart", "orders", "shipping",
           "ledger", "reports"]
DECL = re.compile(r"\b([a-z_][a-z0-9_]*)\(")
TEST = re.compile(r"^TEST\((\w+),\s*(\w+)\)")


def declared_functions(header: Path) -> set[str]:
    names: set[str] = set()
    statement = ""
    for raw in header.read_text().splitlines():
        line = raw.split("//")[0].rstrip()
        if not line.strip():
            continue
        if raw.startswith((" ", "\t")) and not statement:
            continue  # struct/class members, enum values
        statement += " " + line.strip()
        if statement.endswith(";") or statement.endswith("{") or statement.endswith("}"):
            text = statement.strip()
            statement = ""
            if not text.endswith(";") or "(" not in text:
                continue
            if text.startswith(("using", "inline", "class", "struct", "enum", "#", "namespace")):
                continue
            if "operator" in text:
                continue
            match = DECL.search(text)
            if match:
                names.add(match.group(1))
    return names


def definitions(source: Path, public: set[str]) -> list[dict]:
    lines = source.read_text().splitlines()
    out = []
    in_anonymous = False
    i = 0
    while i < len(lines):
        line = lines[i]
        if line == "namespace {":
            in_anonymous = True
        elif line.startswith("}  // namespace") and in_anonymous:
            in_anonymous = False
        elif (not in_anonymous and line and not line[0].isspace()
              and not line.startswith(("}", "#", "//", "namespace", "using", "static", "const"))):
            match = DECL.search(line)
            if match and match.group(1) in public and "operator" not in line:
                end = i
                while end < len(lines) and lines[end] != "}":
                    end += 1
                out.append({"name": match.group(1), "start_line": i + 1, "end_line": end + 1})
                i = end
        i += 1
    return out


def tests_in(test_file: Path) -> list[str]:
    if not test_file.exists():
        return []
    return [f"{m.group(1)}.{m.group(2)}" for m in map(TEST.match, test_file.read_text().splitlines())
            if m]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    parser.add_argument("--output", default=".virtint/functions.json")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    root = Path(args.root)

    functions: dict[str, dict] = {}
    problems: list[str] = []
    for module in MODULES:
        header = root / "src" / "shopcore" / module / f"{module}.hpp"
        public = declared_functions(header)
        defined: set[str] = set()
        for source in sorted((root / "src" / "shopcore" / module).glob("*.cpp")):
            rel_source = str(source.relative_to(root))
            defs = definitions(source, public)
            test_file = root / "tests" / module / f"{source.stem}_test.cpp"
            names = sorted({d["name"] for d in defs}, key=len, reverse=True)
            by_name: dict[str, list[str]] = {n: [] for n in names}
            for test_id in tests_in(test_file):
                case = test_id.split(".", 1)[1]
                owner = next((n for n in names if case.startswith(n + "_")), None)
                if owner is None and case.startswith("stream_operator_"):
                    continue  # operator<< overloads are logging helpers, not indexed
                if owner is None:
                    problems.append(f"{test_file.relative_to(root)}: {test_id} matches no function")
                else:
                    by_name[owner].append(test_id)
            for d in defs:
                qualified = f"shopcore::{module}::{d['name']}"
                if qualified in functions:
                    problems.append(f"{qualified} defined twice")
                functions[qualified] = {
                    "module": module,
                    "file": rel_source,
                    "start_line": d["start_line"],
                    "end_line": d["end_line"],
                    "tests": by_name[d["name"]],
                    "test_command": "make test TESTS=\"" + ";".join(by_name[d["name"]]) + "\"",
                }
                defined.add(d["name"])
        for name in sorted(public - defined):
            problems.append(f"shopcore::{module}::{name} is declared but no definition was found")

    for name, info in functions.items():
        if len(info["tests"]) < 3:
            problems.append(f"{name} has only {len(info['tests'])} test(s)")

    output = root / args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(dict(sorted(functions.items())), indent=2) + "\n")
    total_tests = sum(len(f["tests"]) for f in functions.values())
    print(f"functions.json: {len(functions)} public functions, {total_tests} mapped tests")
    for problem in problems:
        print("  problem: " + problem, file=sys.stderr)
    return 1 if (args.check and problems) else 0


if __name__ == "__main__":
    sys.exit(main())
