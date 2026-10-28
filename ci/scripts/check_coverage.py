#!/usr/bin/env python3
"""Gate component line and branch coverage using native LLVM reports and the component blacklist.

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
        "--skip-report", "--skip-gcovr",
        dest="skip_report",
        action="store_true",
        help="Reuse existing LLVM coverage/coverage.json instead of collecting profiles",
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


def cmake_cache_value(build_dir: Path, key: str) -> str | None:
    cache = build_dir / "CMakeCache.txt"
    if not cache.is_file():
        return None
    for line in cache.read_text().splitlines():
        if line.startswith(key + ":"):
            _, _, value = line.partition("=")
            value = value.strip()
            if value:
                return value
    return None


def run_llvm_coverage(build_dir: Path, coverage_dir: Path) -> Path:
    """Merge test profiles, then let LLVM produce both the gate data and HTML."""
    compiler = cmake_cache_value(build_dir, "CMAKE_CXX_COMPILER")
    if not compiler or cmake_cache_value(build_dir, "COVERAGE") != "ON":
        raise SystemExit("error: configure and build the unit-test-coverage preset first")
    profdata = os.environ.get("LLVM_PROFDATA") or str(Path(compiler).resolve().with_name("llvm-profdata"))
    cov = profdata.replace("llvm-profdata", "llvm-cov")
    if not shutil.which(profdata) or not shutil.which(cov):
        raise SystemExit("error: LLVM coverage tools missing; set LLVM_PROFDATA to the tool matching your Clang")

    tests = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(build_dir), "--show-only=json-v1"], text=True,
    ))
    binaries = set()
    for test in tests["tests"]:
        properties = {p["name"]: p["value"] for p in test.get("properties", [])}
        if not properties.get("DISABLED") and any(
            value.startswith("LLVM_PROFILE_FILE=") for value in properties.get("ENVIRONMENT", [])
        ):
            binaries.add(Path(test["command"][0]))
    if not binaries:
        raise SystemExit("error: no coverage tests found; configure and build the coverage preset first")

    profiles = []
    for binary in sorted(binaries):
        # Ignore counters left by an earlier build of this executable.
        current = [p for p in (coverage_dir / "profiles" / binary.name).glob("*.profraw")
                   if p.stat().st_mtime_ns >= binary.stat().st_mtime_ns]
        if not current:
            raise SystemExit(f"error: no current profiles for {binary.name}; run ctest --preset unit-test-coverage")
        profiles.extend(sorted(current))
    coverage_dir.mkdir(parents=True, exist_ok=True)
    merged = coverage_dir / "coverage.profdata"
    print(f"Merging LLVM profiles from {len(binaries)} test executables", flush=True)
    subprocess.run([profdata, "merge", "-sparse", *map(str, profiles), "-o", str(merged)], check=True)

    # LLVM merges shared files and inline instantiations across these binaries.
    objects = [arg for binary in sorted(binaries) for arg in ["-object", str(binary)]]
    common = [*objects, "-instr-profile=" + str(merged)]
    # Ask LLVM which files are mapped, then use the same source scope for HTML and JSON.
    mapped = json.loads(subprocess.check_output([cov, "export", *common, "-summary-only"], text=True))
    sources = sorted({entry["filename"] for dataset in mapped["data"] for entry in dataset["files"]
                      if posix_rel(entry["filename"], REPO_ROOT).startswith("components/")
                      and not should_skip_file(posix_rel(entry["filename"], REPO_ROOT))})
    if not sources:
        raise SystemExit("error: LLVM found no component source files in the test executables")
    common.extend(["--sources", *sources])
    json_path = coverage_dir / "coverage.json"
    print("Exporting LLVM file summaries for the component gates", flush=True)
    with json_path.open("w") as output:
        subprocess.run([cov, "export", *common, "-summary-only"], stdout=output, check=True)
    lcov_path = coverage_dir / "lcov.info"
    print("Exporting LCOV report: " + str(lcov_path), flush=True)
    with lcov_path.open("w") as output:
        subprocess.run([cov, "export", *common, "-format=lcov"], stdout=output, check=True)
    print("Writing LLVM HTML report: " + str(coverage_dir / "html/index.html"), flush=True)
    subprocess.run([
        cov, "show", *common, "-format=html", "-output-dir=" + str(coverage_dir / "html"),
        "-show-branches=count", "-show-expansions", "-show-branch-summary",
    ], check=True)
    return json_path


def aggregate(json_path: Path, prefixes: list[str]) -> tuple[dict[str, tuple[int, int, int, int]], list[str]]:
    payload = json.loads(json_path.read_text())
    if payload.get("type") != "llvm.coverage.json.export":
        raise SystemExit("error: expected a native LLVM coverage report; rerun without --skip-report")
    totals: dict[str, list[int]] = defaultdict(lambda: [0, 0, 0, 0])
    unmatched: list[str] = []
    for dataset in payload["data"]:
        for entry in dataset["files"]:
            rel = posix_rel(entry["filename"], REPO_ROOT)
            if not rel.startswith("components/") or should_skip_file(rel):
                continue
            prefix = longest_prefix(rel, prefixes)
            if prefix is None:
                unmatched.append(rel)
                continue
            summary = entry["summary"]
            totals[prefix][0] += summary["lines"]["covered"]
            totals[prefix][1] += summary["lines"]["count"]
            totals[prefix][2] += summary["branches"]["covered"]
            totals[prefix][3] += summary["branches"]["count"]
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
        if line_pct is None:
            result = "❌ No data"
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
            summary.write("## Component line and branch coverage\n\n")
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
    if args.skip_report:
        if not json_path.is_file():
            print(f"error: {json_path} missing; run without --skip-report", file=sys.stderr)
            return 2
    else:
        json_path = run_llvm_coverage(build_dir, coverage_dir)

    totals, unmatched = aggregate(json_path, prefixes)
    report_table(coverage_table(prefixes, totals, blacklist, min_line, min_branch))

    failures: list[str] = []
    for prefix in sorted(set(prefixes) | set(totals)):
        if prefix in blacklist:
            continue
        lines_covered, lines_total, branches_covered, branches_total = totals.get(prefix, (0, 0, 0, 0))
        if not lines_total:
            failures.append(f"{prefix}: no executable line coverage data; configure, build, and run its tests")
            continue
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
