#!/usr/bin/env python3
"""Gate component line coverage using gcovr and ci/coverage/component_blacklist.yaml.

From the repository root, after cmake --workflow --preset unit-test-coverage:

    python3 ci/scripts/check_coverage.py --build-dir build/unit-test-coverage
"""
from __future__ import annotations

import argparse
import json
import os
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
        help="YAML with min_line_percent and blacklist paths",
    )
    parser.add_argument(
        "--skip-gcovr",
        action="store_true",
        help="Reuse existing coverage/coverage.json instead of running gcovr",
    )
    return parser.parse_args()


def load_config(path: Path) -> tuple[float, list[str]]:
    data = yaml.safe_load(path.read_text())
    minimum = float(data["min_line_percent"])
    blacklist = [normalize_prefix(p) for p in data["blacklist"]]
    return minimum, blacklist


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

    A child of a blacklisted parent (e.g. a command class) is gated on its own
    path when it has a test/ directory, so unblacklisting it is enough. crc16
    is gated the same way: it is omitted from the YAML and has tests.
    """
    prefixes = set(blacklist)
    components = repo / "components"
    if components.is_dir():
        for child in components.iterdir():
            if _is_tested_component(child):
                prefixes.add(normalize_prefix(str(child.relative_to(repo))))
    for parent in blacklist:
        path = repo / parent
        if not path.is_dir():
            continue
        for child in path.iterdir():
            if _is_tested_component(child):
                prefixes.add(normalize_prefix(str(child.relative_to(repo))))
    return sorted(prefixes)


def run_gcovr(build_dir: Path, coverage_dir: Path) -> Path:
    coverage_dir.mkdir(parents=True, exist_ok=True)
    json_path = coverage_dir / "coverage.json"
    xml_path = coverage_dir / "coverage.xml"
    html_dir = coverage_dir / "html"
    html_dir.mkdir(parents=True, exist_ok=True)
    common = [
        "gcovr",
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


def aggregate(json_path: Path, prefixes: list[str]) -> tuple[dict[str, tuple[int, int]], list[str]]:
    payload = json.loads(json_path.read_text())
    files = payload.get("files", [])
    totals: dict[str, list[int]] = defaultdict(lambda: [0, 0])
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
        totals[prefix][0] += covered
        totals[prefix][1] += total
    return {k: (v[0], v[1]) for k, v in totals.items()}, unmatched


def percent(covered: int, total: int) -> float:
    if total == 0:
        return 100.0
    return 100.0 * covered / total


def write_summary(lines: list[str]) -> None:
    text = "\n".join(lines) + "\n"
    sys.stdout.write(text)
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as handle:
            handle.write(text)


def main() -> int:
    args = parse_args()
    build_dir = args.build_dir if args.build_dir.is_absolute() else REPO_ROOT / args.build_dir
    blacklist_path = args.blacklist if args.blacklist.is_absolute() else REPO_ROOT / args.blacklist
    coverage_dir = build_dir / "coverage"

    minimum, blacklist = load_config(blacklist_path)
    prefixes = discover_prefixes(REPO_ROOT, blacklist)

    json_path = coverage_dir / "coverage.json"
    if args.skip_gcovr:
        if not json_path.is_file():
            print(f"error: {json_path} missing; run without --skip-gcovr", file=sys.stderr)
            return 2
    else:
        json_path = run_gcovr(build_dir, coverage_dir)

    totals, unmatched = aggregate(json_path, prefixes)

    rows: list[str] = [
        "### Unit test coverage",
        "",
        f"Minimum line coverage for non-blacklisted components: {minimum:g}%",
        "",
        "| Component | Lines | Covered | % | Status |",
        "|-----------|------:|--------:|--:|--------|",
    ]
    failures: list[str] = []
    for prefix in sorted(set(prefixes) | set(totals)):
        covered, total = totals.get(prefix, (0, 0))
        pct = percent(covered, total)
        blacklisted = prefix in blacklist
        if total == 0 and blacklisted:
            status = "blacklist (no data)"
        elif blacklisted:
            status = "blacklist"
            if pct >= minimum:
                status = "blacklist (already ≥ gate; remove from YAML)"
                print(f"warning: {prefix} is {pct:.1f}% covered; remove it from the blacklist", file=sys.stderr)
        elif total == 0:
            status = "gated (no executable lines)"
        elif pct + 1e-9 < minimum:
            status = "FAIL"
            failures.append(f"{prefix}: {pct:.1f}% ({covered}/{total}) < {minimum:g}%")
        else:
            status = "pass"
        rows.append(f"| `{prefix}` | {total} | {covered} | {pct:.1f} | {status} |")

    write_summary(rows)

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
