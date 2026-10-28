#!/usr/bin/env python3
"""Merge the Debug and unit-test compilation databases for editor use."""

import argparse
import json
import os
import sys
import tempfile
from pathlib import Path


def source_path(entry):
    source = Path(entry["file"])
    if not source.is_absolute():
        source = Path(entry["directory"]) / source
    return source.resolve()


def main():
    repo_root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--debug",
        type=Path,
        default=repo_root / "build/debug/compile_commands.json",
    )
    parser.add_argument(
        "--unit-test",
        type=Path,
        default=repo_root / "build/unit-test/compile_commands.json",
    )
    parser.add_argument(
        "--output", type=Path, default=repo_root / "compile_commands.json"
    )
    args = parser.parse_args()

    if sys.prefix == sys.base_prefix:
        print("warning: compile database merger is not running in a virtual environment", file=sys.stderr)

    if not args.debug.is_file():
        print(
            f"warning: cannot merge compilation databases; missing: {args.debug}",
            file=sys.stderr,
        )
        return 0

    # Debug entries win when both presets compile the same source file.
    entries_by_source = {}
    databases = [args.debug, args.unit_test]
    for database in databases:
        if not database.is_file():
            continue
        with database.open(encoding="utf-8") as stream:
            entries = json.load(stream)
        if not isinstance(entries, list):
            raise ValueError(f"{database} must contain a JSON array")
        for entry in entries:
            entries_by_source.setdefault(source_path(entry), entry)

    entries = [entries_by_source[key] for key in sorted(entries_by_source)]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    temporary_path = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w",
            encoding="utf-8",
            dir=args.output.parent,
            prefix=f".{args.output.name}.",
            suffix=".tmp",
            delete=False,
        ) as stream:
            temporary_path = Path(stream.name)
            json.dump(entries, stream, indent=2)
            stream.write("\n")
        os.replace(temporary_path, args.output)
    finally:
        if temporary_path is not None and temporary_path.exists():
            temporary_path.unlink()

    print(f"Merged {len(entries)} compile commands into {args.output}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, json.JSONDecodeError, KeyError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
