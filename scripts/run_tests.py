#!/usr/bin/env python3
"""Runs the ShopCore GoogleTest binaries and writes JUnit XML to reports/junit/.

Usage: run_tests.py --build-dir build --reports reports [--tests "Suite.name;Suite.name"]

Exit codes (written to reports/exit-code by the Makefile):
  0  every selected test passed
  1  at least one test failed (or crashed / timed out)
  2  infrastructure error: no test binaries, a requested test ID does not exist, ...

A binary that crashes or hangs is re-run one test at a time so the other tests in
it still get a result; the crashing/hanging test is reported as an <error>.
Stdlib only: no dependencies to install before grading.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

BINARY_TIMEOUT_S = 45  # the whole suite runs in about a second
SINGLE_TEST_TIMEOUT_S = 10


def list_tests(binary: Path) -> list[str]:
    out = subprocess.run([str(binary), "--gtest_list_tests"], capture_output=True, text=True,
                         timeout=60, check=True).stdout
    tests: list[str] = []
    suite = ""
    for line in out.splitlines():
        if not line.strip():
            continue
        if not line.startswith(" "):
            suite = line.strip().split()[0].rstrip(".")
        else:
            tests.append(f"{suite}.{line.strip().split()[0]}")
    return tests


def run_binary(binary: Path, gtest_filter: str, xml_path: Path, timeout: int) -> tuple[int | None, str]:
    """Returns (exit code or None on timeout, combined output)."""
    if xml_path.exists():
        xml_path.unlink()
    cmd = [str(binary), f"--gtest_filter={gtest_filter}", f"--gtest_output=xml:{xml_path}",
           "--gtest_color=no"]
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout,
                              errors="replace")
        return proc.returncode, proc.stdout + proc.stderr
    except subprocess.TimeoutExpired as exc:
        out = (exc.stdout or b"") if isinstance(exc.stdout, bytes) else (exc.stdout or "")
        if isinstance(out, bytes):
            out = out.decode(errors="replace")
        return None, out


def xml_complete(xml_path: Path) -> bool:
    try:
        ET.parse(xml_path)
        return True
    except (ET.ParseError, FileNotFoundError):
        return False


def synthesize_xml(binary: Path, tests: list[str], workdir: Path, xml_path: Path) -> None:
    """Runs tests one by one and merges the results into a single JUnit file."""
    suites: dict[str, ET.Element] = {}
    root = ET.Element("testsuites", name="AllTests")
    for test_id in tests:
        suite_name, name = test_id.split(".", 1)
        single = workdir / f"{binary.name}.single.xml"
        code, output = run_binary(binary, test_id, single, SINGLE_TEST_TIMEOUT_S)
        case = None
        if code is not None and xml_complete(single):
            for found in ET.parse(single).getroot().iter("testcase"):
                case = found
        if case is None:
            case = ET.Element("testcase", name=name, classname=suite_name, status="run",
                              result="completed", time="0")
            reason = "timed out" if code is None else f"crashed (exit code {code})"
            err = ET.SubElement(case, "error", message=f"{test_id} {reason}", type="")
            err.text = output[-4000:]
        if suite_name not in suites:
            suites[suite_name] = ET.SubElement(root, "testsuite", name=suite_name)
        suites[suite_name].append(case)
        if single.exists():
            single.unlink()
    for suite in suites.values():
        cases = list(suite.iter("testcase"))
        suite.set("tests", str(len(cases)))
        suite.set("failures", str(sum(1 for c in cases if c.find("failure") is not None)))
        suite.set("errors", str(sum(1 for c in cases if c.find("error") is not None)))
    ET.ElementTree(root).write(xml_path, encoding="utf-8", xml_declaration=True)


def count_results(xml_path: Path) -> tuple[int, int, set[str]]:
    total = failed = 0
    ids: set[str] = set()
    for case in ET.parse(xml_path).getroot().iter("testcase"):
        if case.get("status", "run") == "notrun" or case.get("result") == "skipped":
            continue
        total += 1
        ids.add(f"{case.get('classname')}.{case.get('name')}")
        if case.find("failure") is not None or case.find("error") is not None:
            failed += 1
    return total, failed, ids


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--reports", default="reports")
    parser.add_argument("--tests", default="", help="';'-separated Suite.name test IDs")
    args = parser.parse_args()

    test_dir = Path(args.build_dir) / "tests"
    junit_dir = Path(args.reports) / "junit"
    junit_dir.mkdir(parents=True, exist_ok=True)
    for stale in junit_dir.glob("*.xml"):
        stale.unlink()

    binaries = sorted(p for p in test_dir.glob("shopcore_*_tests") if os.access(p, os.X_OK))
    if not binaries:
        print(f"run_tests: no test binaries in {test_dir}; run 'make build' first", file=sys.stderr)
        return 2

    requested = [t.strip() for t in args.tests.replace(",", ";").split(";") if t.strip()]
    gtest_filter = ":".join(requested) if requested else "*"

    total = failed = 0
    seen: set[str] = set()
    for binary in binaries:
        xml_path = junit_dir / f"{binary.name}.xml"
        try:
            all_tests = list_tests(binary)
        except (subprocess.SubprocessError, OSError) as exc:
            print(f"run_tests: cannot list tests in {binary.name}: {exc}", file=sys.stderr)
            return 2
        selected = [t for t in all_tests if not requested or t in requested]
        if not selected:
            continue
        code, output = run_binary(binary, gtest_filter, xml_path, BINARY_TIMEOUT_S)
        sys.stdout.write(output)
        if code is None or not xml_complete(xml_path) or code not in (0, 1):
            print(f"run_tests: {binary.name} crashed or timed out; re-running tests one by one",
                  file=sys.stderr)
            synthesize_xml(binary, selected, junit_dir, xml_path)
        t, f, ids = count_results(xml_path)
        total += t
        failed += f
        seen |= ids

    missing = [t for t in requested if t not in seen]
    print(f"run_tests: {total} tests, {failed} failed")
    if missing:
        print("run_tests: unknown test IDs: " + ";".join(missing), file=sys.stderr)
        return 2
    if total == 0:
        print("run_tests: no tests were run", file=sys.stderr)
        return 2
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
