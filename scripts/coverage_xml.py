#!/usr/bin/env python3
"""Converts gcov line coverage of src/shopcore into a Cobertura XML report.

Usage: coverage_xml.py --build-dir build-coverage --source-root . --output reports/coverage.xml

Runs `gcov --json-format` on every .gcda under the build directory (gcc >= 9), merges
the per-translation-unit results (headers appear in several TUs) and writes Cobertura
XML with one <class> per source file. Stdlib only, so it works offline in the grading
image; set GCOV to choose a gcov binary matching the compiler.
"""

from __future__ import annotations

import argparse
import gzip
import json
import os
import subprocess
import sys
import tempfile
import time
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path


def collect(build_dir: Path, source_root: Path, gcov: str) -> dict[str, dict[int, int]]:
    gcdas = sorted(build_dir.rglob("*.gcda"))
    if not gcdas:
        raise SystemExit(f"coverage_xml: no .gcda files under {build_dir}; did the tests run?")
    src_dir = (source_root / "src").resolve()
    lines: dict[str, dict[int, int]] = defaultdict(dict)
    with tempfile.TemporaryDirectory() as tmp:
        # gcov writes <name>.gcov.json.gz into its working directory.
        # Process one .gcda at a time so reports from same-named sources never collide.
        for gcda in gcdas:
            subprocess.run([gcov, "--json-format", str(gcda.resolve())], cwd=tmp,
                           capture_output=True, check=True)
            for report in list(Path(tmp).glob("*.gcov.json.gz")):
                with gzip.open(report, "rt", encoding="utf-8") as fh:
                    data = json.load(fh)
                report.unlink()
                merge(data, src_dir, lines)
    return lines


def merge(data: dict, src_dir: Path, lines: dict[str, dict[int, int]]) -> None:
    cwd = Path(data.get("current_working_directory", "."))
    for entry in data.get("files", []):
        path = Path(entry["file"])
        if not path.is_absolute():
            path = cwd / path
        path = path.resolve()
        try:
            rel = path.relative_to(src_dir)
        except ValueError:
            continue
        key = str(Path("src") / rel)
        for line in entry.get("lines", []):
            number = int(line["line_number"])
            lines[key][number] = lines[key].get(number, 0) + int(line["count"])


def rate(covered: int, valid: int) -> str:
    return f"{(covered / valid) if valid else 1.0:.4f}"


def write_cobertura(lines: dict[str, dict[int, int]], output: Path, source_root: Path) -> tuple[int, int]:
    total_valid = sum(len(v) for v in lines.values())
    total_covered = sum(1 for v in lines.values() for c in v.values() if c > 0)
    root = ET.Element("coverage", {
        "line-rate": rate(total_covered, total_valid), "branch-rate": "0",
        "lines-covered": str(total_covered), "lines-valid": str(total_valid),
        "branches-covered": "0", "branches-valid": "0", "complexity": "0",
        "version": "shopcore-coverage-1", "timestamp": str(int(time.time())),
    })
    sources = ET.SubElement(root, "sources")
    ET.SubElement(sources, "source").text = str(source_root.resolve())
    packages = ET.SubElement(root, "packages")
    by_package: dict[str, list[str]] = defaultdict(list)
    for filename in sorted(lines):
        by_package[str(Path(filename).parent).replace(os.sep, ".")].append(filename)
    for package_name, files in sorted(by_package.items()):
        p_valid = sum(len(lines[f]) for f in files)
        p_cov = sum(1 for f in files for c in lines[f].values() if c > 0)
        package = ET.SubElement(packages, "package", {
            "name": package_name, "line-rate": rate(p_cov, p_valid), "branch-rate": "0",
            "complexity": "0"})
        classes = ET.SubElement(package, "classes")
        for filename in files:
            file_lines = lines[filename]
            covered = sum(1 for c in file_lines.values() if c > 0)
            cls = ET.SubElement(classes, "class", {
                "name": Path(filename).name, "filename": filename,
                "line-rate": rate(covered, len(file_lines)), "branch-rate": "0", "complexity": "0"})
            ET.SubElement(cls, "methods")
            xml_lines = ET.SubElement(cls, "lines")
            for number in sorted(file_lines):
                ET.SubElement(xml_lines, "line", {"number": str(number), "hits": str(file_lines[number]),
                                                  "branch": "false"})
    output.parent.mkdir(parents=True, exist_ok=True)
    ET.ElementTree(root).write(output, encoding="utf-8", xml_declaration=True)
    return total_covered, total_valid


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default="build-coverage")
    parser.add_argument("--source-root", default=".")
    parser.add_argument("--output", default="reports/coverage.xml")
    parser.add_argument("--summary", action="store_true", help="print a per-file table")
    args = parser.parse_args()
    lines = collect(Path(args.build_dir), Path(args.source_root), os.environ.get("GCOV", "gcov"))
    covered, valid = write_cobertura(lines, Path(args.output), Path(args.source_root))
    if args.summary:
        for filename in sorted(lines):
            fl = lines[filename]
            c = sum(1 for h in fl.values() if h > 0)
            print(f"  {filename:50s} {c:4d}/{len(fl):4d}  {100.0 * c / max(1, len(fl)):6.2f}%")
    print(f"coverage: {covered}/{valid} lines = {100.0 * covered / max(1, valid):.2f}%")
    return 0


if __name__ == "__main__":
    sys.exit(main())
