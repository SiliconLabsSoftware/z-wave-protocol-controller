#!/usr/bin/env python3
"""Gate component line and branch coverage using gcovr and the component blacklist.

From the repository root, after cmake --workflow --preset unit-test-coverage:

    python3 ci/scripts/check_coverage.py --build-dir build/unit-test-coverage
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

import yaml

REPO_ROOT = Path(__file__).resolve().parent.parent.parent


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=REPO_ROOT / "build" / "unit-test-coverage",
        help="Coverage CMake binary dir (default: build/unit-test-coverage)",
    )
    parser.add_argument(
        "--blacklist",
        type=Path,
        default=REPO_ROOT / "ci" / "coverage" / "component_blacklist.yaml",
        help="YAML listing component paths exempt from coverage checks",
    )
    parser.add_argument(
        "--thresholds",
        type=Path,
        default=REPO_ROOT / "ci" / "unit_test_config.yaml",
        help="YAML with unit-test configuration, including coverage minimums",
    )
    parser.add_argument(
        "--skip-gcovr",
        action="store_true",
        help="Reuse existing coverage/coverage.json instead of running gcovr",
    )
    return parser.parse_args()


def load_blacklist(path: Path) -> list[str]:
    data = yaml.safe_load(path.read_text())
    return [normalize_prefix(p) for p in data["blacklist"]]


def load_thresholds(path: Path) -> tuple[float, float]:
    data = yaml.safe_load(path.read_text())
    return float(data["min_line_percent"]), float(data["min_branch_percent"])


def normalize_prefix(path: str) -> str:
    return path.replace("\\", "/").rstrip("/")


def posix_rel(path: str, root: Path) -> str:
    resolved = Path(path)
    if not resolved.is_absolute():
        resolved = (root / resolved).resolve()
    try:
        rel = resolved.resolve().relative_to(root.resolve())
    except ValueError:
        return normalize_prefix(str(resolved))
    return normalize_prefix(str(rel))


def should_skip_file(rel: str) -> bool:
    parts = rel.split("/")
    if "generated" in parts:
        return True
    if "test" in parts:
        return True
    if "_deps" in parts or "googletest" in parts:
        return True
    return False


def longest_prefix(rel: str, prefixes: list[str]) -> str | None:
    best: str | None = None
    for prefix in prefixes:
        if rel == prefix or rel.startswith(prefix + "/"):
            if best is None or len(prefix) > len(best):
                best = prefix
    return best


def _is_tested_component(path: Path) -> bool:
    return path.is_dir() and (path / "CMakeLists.txt").is_file() and (path / "test").is_dir()


def discover_prefixes(repo: Path, blacklist: list[str]) -> list[str]:
    """Blacklist paths plus tested components omitted from the YAML.

    Find tested components recursively so nested libraries and command classes
    under a blacklisted parent are gated on their own paths when they have a
    test/ directory. crc16 is gated the same way: it is omitted from the YAML.
    """
    prefixes = set(blacklist)
    components = repo / "components"
    if components.is_dir():
        for cmake_file in components.rglob("CMakeLists.txt"):
            component = cmake_file.parent
            if _is_tested_component(component):
                prefixes.add(normalize_prefix(str(component.relative_to(repo))))
    return sorted(prefixes)


def cmake_c_compiler(build_dir: Path) -> str | None:
    cache = build_dir / "CMakeCache.txt"
    if not cache.is_file():
        return None
    for line in cache.read_text().splitlines():
        if line.startswith("CMAKE_C_COMPILER:"):
            _, _, value = line.partition("=")
            value = value.strip()
            if value:
                return value
    return None


def gcov_from_compiler(compiler: str | None) -> str | None:
    if not compiler:
        return None
    name = Path(compiler.split()[0]).name
    if "gcc" not in name:
        return None
    return name.replace("gcc", "gcov", 1)


def gcovr_gcov_args(build_dir: Path) -> list[str]:
    """Use the gcov that matches the compiler that wrote the .gcno files.

    CI builds coverage with gcc-14 (notes version B42*). Default `gcov` on
    ubuntu-24.04 is gcc-13 (B33*), which makes gcovr abort on the first .gcda.
    """
    gcov = os.environ.get("GCOV") or gcov_from_compiler(cmake_c_compiler(build_dir)) or gcov_from_compiler(os.environ.get("CC"))
    if not gcov:
        return []
    executable = gcov.split()[0]
    if shutil.which(executable) is None:
        raise SystemExit(f"error: gcov executable {executable!r} not found; install the matching gcc package or set GCOV")
    return ["--gcov-executable", gcov]


def run_gcovr(build_dir: Path, coverage_dir: Path) -> Path:
    coverage_dir.mkdir(parents=True, exist_ok=True)
    json_path = coverage_dir / "coverage.json"
    xml_path = coverage_dir / "coverage.xml"
    html_dir = coverage_dir / "html"
    html_dir.mkdir(parents=True, exist_ok=True)
    gcov_args = gcovr_gcov_args(build_dir)
    common = [
        "gcovr",
        *gcov_args,
        "--root",
        str(REPO_ROOT),
        "--filter",
        "components/",
        "--exclude",
        ".*/generated/.*",
        "--exclude",
        ".*/_deps/.*",
        "--exclude",
        ".*/googletest/.*",
        "--json",
        str(json_path),
        "--xml",
        str(xml_path),
        "--html-details",
        str(html_dir / "index.html"),
        "--print-summary",
    ]
    obj_dir = str(build_dir.resolve())
    for flag in ("--gcov-object-directory", "--object-directory"):
        result = subprocess.run(common[:1] + [flag, obj_dir] + common[1:], cwd=REPO_ROOT)
        if result.returncode == 0:
            return json_path
    raise SystemExit("error: gcovr failed (tried --gcov-object-directory and --object-directory)")


def file_line_counts(entry: dict) -> tuple[int, int]:
    lines = entry.get("lines")
    if not lines:
        covered = int(entry.get("line_covered", 0))
        total = int(entry.get("line_total", 0))
        return covered, total
    executable = 0
    covered = 0
    for line in lines:
        if line.get("gcovr/noncode"):
            continue
        count = line.get("count")
        if count is None:
            continue
        executable += 1
        if count > 0:
            covered += 1
    return covered, executable


def file_branch_counts(entry: dict) -> tuple[int, int]:
    covered = 0
    total = 0
    for line in entry.get("lines", []):
        if line.get("gcovr/noncode"):
            continue
        for branch in line.get("branches", []):
            count = branch.get("count")
            if count is None:
                continue
            total += 1
            if count > 0:
                covered += 1
    return covered, total


def aggregate(json_path: Path, prefixes: list[str]) -> tuple[dict[str, tuple[int, int, int, int]], list[str]]:
    payload = json.loads(json_path.read_text())
    files = payload.get("files", [])
    totals: dict[str, list[int]] = defaultdict(lambda: [0, 0, 0, 0])
    unmatched: list[str] = []
    for entry in files:
        rel = posix_rel(entry.get("file", ""), REPO_ROOT)
        if not rel.startswith("components/"):
            continue
        if should_skip_file(rel):
            continue
        prefix = longest_prefix(rel, prefixes)
        if prefix is None:
            unmatched.append(rel)
            continue
        covered, total = file_line_counts(entry)
        branches_covered, branches_total = file_branch_counts(entry)
        totals[prefix][0] += covered
        totals[prefix][1] += total
        totals[prefix][2] += branches_covered
        totals[prefix][3] += branches_total
    return {k: (v[0], v[1], v[2], v[3]) for k, v in totals.items()}, unmatched


def percent(covered: int, total: int) -> float:
    if total == 0:
        return 100.0
    return 100.0 * covered / total


def coverage_table(
    prefixes: list[str],
    totals: dict[str, tuple[int, int, int, int]],
    blacklist: list[str],
    min_line: float,
    min_branch: float,
) -> str:
    rows = ["| Component | Line coverage | Branch coverage | Result |", "|---|---:|---:|---|"]
    for prefix in sorted((set(prefixes) | set(totals)) - set(blacklist)):
        lines_covered, lines_total, branches_covered, branches_total = totals.get(prefix, (0, 0, 0, 0))
        line_pct = percent(lines_covered, lines_total) if lines_total else None
        branch_pct = percent(branches_covered, branches_total) if branches_total else None
        line_coverage = f"{line_pct:.1f}% ({lines_covered}/{lines_total})" if line_pct is not None else "—"
        branch_coverage = f"{branch_pct:.1f}% ({branches_covered}/{branches_total})" if branch_pct is not None else "—"
        if line_pct is None and branch_pct is None:
            result = "No data"
        elif (line_pct is None or line_pct + 1e-9 >= min_line) and (branch_pct is None or branch_pct + 1e-9 >= min_branch):
            result = "✅ Pass"
        else:
            result = "❌ Fail"
        rows.append(f"| `{prefix}` | {line_coverage} | {branch_coverage} | {result} |")
    return "\n".join(rows) + "\n"


def report_table(table: str) -> None:
    print(table, end="")
    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary_path:
        with open(summary_path, "a", encoding="utf-8") as summary:
            summary.write("## Component line coverage\n\n")
            summary.write(table)


def main() -> int:
    args = parse_args()
    build_dir = args.build_dir if args.build_dir.is_absolute() else REPO_ROOT / args.build_dir
    blacklist_path = args.blacklist if args.blacklist.is_absolute() else REPO_ROOT / args.blacklist
    thresholds_path = args.thresholds if args.thresholds.is_absolute() else REPO_ROOT / args.thresholds
    coverage_dir = build_dir / "coverage"

    blacklist = load_blacklist(blacklist_path)
    min_line, min_branch = load_thresholds(thresholds_path)
    prefixes = discover_prefixes(REPO_ROOT, blacklist)

    json_path = coverage_dir / "coverage.json"
    if args.skip_gcovr:
        if not json_path.is_file():
            print(f"error: {json_path} missing; run without --skip-gcovr", file=sys.stderr)
            return 2
    else:
        json_path = run_gcovr(build_dir, coverage_dir)

    totals, unmatched = aggregate(json_path, prefixes)
    report_table(coverage_table(prefixes, totals, blacklist, min_line, min_branch))

    failures: list[str] = []
    for prefix in sorted(set(prefixes) | set(totals)):
        if prefix in blacklist:
            continue
        lines_covered, lines_total, branches_covered, branches_total = totals.get(prefix, (0, 0, 0, 0))
        if lines_total and percent(lines_covered, lines_total) + 1e-9 < min_line:
            failures.append(
                f"{prefix}: line {percent(lines_covered, lines_total):.1f}% "
                f"({lines_covered}/{lines_total}) < {min_line:g}%"
            )
        if branches_total and percent(branches_covered, branches_total) + 1e-9 < min_branch:
            failures.append(
                f"{prefix}: branch {percent(branches_covered, branches_total):.1f}% "
                f"({branches_covered}/{branches_total}) < {min_branch:g}%"
            )

    if unmatched:
        print("error: source files under components/ matched no known component prefix:", file=sys.stderr)
        for rel in unmatched:
            print(f"  {rel}", file=sys.stderr)
        print("Add the component path to ci/coverage/component_blacklist.yaml (until tests exist).", file=sys.stderr)
        return 1

    if failures:
        print("error: coverage gate failed:", file=sys.stderr)
        for item in failures:
            print(f"  {item}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
