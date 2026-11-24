#!/usr/bin/env python3
"""Run run-clang-tidy locally with percent progress; print errors when done."""

import argparse
import json
import os
import re
import subprocess
import sys

PROGRESS_RE = re.compile(r"^\[\s*(\d+)/(\d+)\]")
TOTAL_RE = re.compile(r"for (\d+) files")
PATH_FILTERS = ("applications", "components")


def count_files(compile_db):
    if not compile_db or not os.path.isfile(compile_db):
        return 0
    with open(compile_db) as handle:
        db = json.load(handle)
    pat = re.compile("|".join(PATH_FILTERS))
    files = set()
    for entry in db:
        path = entry["file"]
        if not os.path.isabs(path):
            path = os.path.join(entry["directory"], path)
        files.add(os.path.abspath(path))
    n = 0
    for path in files:
        if pat.search(path):
            n += 1
    return n


def print_progress(completed, total):
    if total > 0:
        pct = completed * 100 // total
        sys.stderr.write(f"\rProgress: {completed}/{total} ({pct}%)")
    else:
        sys.stderr.write("\rProgress: starting")
    sys.stderr.flush()


def parse_args(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-db", default="")
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    command = args.command
    if command and command[0] == "--":
        command = command[1:]
    if not command:
        parser.error("missing run-clang-tidy command")
    return args.compile_db, command


def main(argv=None):
    compile_db, command = parse_args(argv)
    completed = 0
    total = count_files(compile_db)
    print_progress(completed, total)

    env = os.environ.copy()
    env["PYTHONUNBUFFERED"] = "1"
    proc = subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env=env,
        text=True,
        bufsize=1,
    )
    stdout = proc.stdout
    if stdout is None:
        return 1

    lines = []
    try:
        for line in stdout:
            lines.append(line)
            match = PROGRESS_RE.match(line)
            if match:
                completed = int(match.group(1))
                total = int(match.group(2))
                print_progress(completed, total)
                continue
            match = TOTAL_RE.search(line)
            if match and completed == 0:
                total = int(match.group(1))
                print_progress(completed, total)
    except KeyboardInterrupt:
        proc.terminate()
        proc.wait()
        sys.stderr.write("\n")
        return 130

    rc = proc.wait()
    print_progress(completed, total)
    sys.stderr.write("\n")
    for line in lines:
        if ": error:" in line:
            sys.stdout.write(line)
    return rc


if __name__ == "__main__":
    sys.exit(main())
