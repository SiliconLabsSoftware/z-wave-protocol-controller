#!/usr/bin/env python3
"""Fail CI on GitHub-unsafe or unparseable ```mermaid fences.

Scans markdown under docs/ and components/**/doc{,s}/, including gitignored
generated copies in docs/api/.

Policy checks (Python) catch syntax that often parses but renders incorrectly on
GitHub and MkDocs Material. mermaid.parse() (Node) catches invalid grammar only.

From the repository root:
    npm install --prefix ci/mermaid
    python3 ci/scripts/check_mermaid.py
    python3 ci/scripts/test_check_mermaid.py
"""
from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
PARSE_SCRIPT = REPO_ROOT / "ci" / "mermaid" / "parse_mermaid.mjs"

FENCE_RE = re.compile(r"^```mermaid[ \t]*\n(.*?)```", re.MULTILINE | re.DOTALL)
HTML_TAG_RE = re.compile(r"</?[a-zA-Z!?][^>]*>")
HTML_ENTITY_RE = re.compile(r"&(?:[a-zA-Z][a-zA-Z0-9]+|#\d+|#x[0-9a-fA-F]+);")
STYLE_RE = re.compile(r"(?m)^\s*style\s+\S+")
CLASSDEF_RE = re.compile(r"(?m)^\s*classDef\b")
CLICK_RE = re.compile(r"(?m)^\s*click\s+")
UNQUOTED_EDGE_RE = re.compile(r"-->\|(?!\")[^|\n]*\([^|\n]*\)[^|\n]*\|")
# Unicode arrows in labels are confused with diagram arrows (e.g. NM-->>NM: → NM_IDLE).
UNICODE_ARROW_RE = re.compile(r"[→←⟶⟹]")
# Colons in unquoted alt/opt/par/loop labels split text on GitHub (alt S2: Keys ...).
ALT_OPT_COLON_RE = re.compile(
    r'(?m)^\s*(?:alt|opt|par|loop|critical)\s+(?!")[^"\n]*:[^"\n]'
)
# Note/ message lines where the first character after ":" is a Unicode arrow.
LABEL_LEADING_ARROW_RE = re.compile(r"(?m)^[^:]+:\s*[→←]")

SKIP_DIR_NAMES = {".git", "node_modules", "html", "build", ".cursor", "site"}
DOC_ROOTS = (
    REPO_ROOT / "docs",
    REPO_ROOT / "components",
)


def iter_markdown_files() -> list[Path]:
    files: list[Path] = []
    for root in DOC_ROOTS:
        if not root.is_dir():
            continue
        for path in root.rglob("*.md"):
            if any(part in SKIP_DIR_NAMES for part in path.parts):
                continue
            if root == REPO_ROOT / "components":
                if "/doc/" not in f"/{path.as_posix()}/" and "/docs/" not in f"/{path.as_posix()}/":
                    continue
            files.append(path)
    return sorted(files)


def _outside_double_quotes(source: str) -> str:
    """Remove double-quoted spans so label punctuation inside quotes is allowed."""
    return re.sub(r'"[^"\\]*(\\.[^"\\]*)*"', "", source)


def extract_fences(path: Path) -> list[tuple[int, str]]:
    text = path.read_text(encoding="utf-8")
    fences: list[tuple[int, str]] = []
    for match in FENCE_RE.finditer(text):
        line = text.count("\n", 0, match.start()) + 1
        fences.append((line, match.group(1)))
    return fences


def banned_issues(source: str) -> list[str]:
    issues: list[str] = []
    if HTML_TAG_RE.search(source):
        issues.append("HTML tags are not GitHub-safe; use extra Note lines or quoted labels with \\n")
    if HTML_ENTITY_RE.search(source):
        issues.append("HTML entities are not GitHub-safe; use plain text in quoted labels")
    if STYLE_RE.search(source):
        issues.append("style statements are not GitHub-safe; omit colors and let the theme apply")
    if CLASSDEF_RE.search(source):
        issues.append("classDef is not GitHub-safe; omit custom styling")
    if CLICK_RE.search(source):
        issues.append("click events are disabled; omit click syntax")
    if UNQUOTED_EDGE_RE.search(source):
        issues.append("quote edge labels that contain parentheses: A -->|\"f()\"| B")
    if UNICODE_ARROW_RE.search(_outside_double_quotes(source)):
        issues.append(
            "Unicode arrows (→, ←, …) are not GitHub-safe in labels; use ASCII "
            '("to", "return to", or "->"), or move them inside double quotes'
        )
    if ALT_OPT_COLON_RE.search(source):
        issues.append(
            'Unquoted colons in alt/opt/par/loop/critical labels break rendering; '
            'reword (e.g. "alt Standard S2 key grant") or quote the label'
        )
    if LABEL_LEADING_ARROW_RE.search(source):
        issues.append(
            "Label text must not start with a Unicode arrow after ':'; "
            'use words (e.g. "return to NM_IDLE")'
        )
    return issues


def main() -> int:
    diagrams: list[dict[str, str | int]] = []
    errors = 0

    for path in iter_markdown_files():
        rel = path.relative_to(REPO_ROOT)
        for line, source in extract_fences(path):
            for issue in banned_issues(source):
                errors += 1
                print(f"{rel}:{line}: {issue}", file=sys.stderr)
            diagrams.append({"file": str(rel), "line": line, "source": source})

    if not diagrams:
        print("No mermaid fences found", file=sys.stderr)
        return 1

    if errors:
        print(f"Banned mermaid syntax in {errors} diagram(s)", file=sys.stderr)
        return 1

    result = subprocess.run(
        ["node", str(PARSE_SCRIPT)],
        input=json.dumps(diagrams),
        text=True,
        cwd=REPO_ROOT / "ci" / "mermaid",
        check=False,
    )
    if result.returncode != 0:
        return result.returncode

    print(f"Checked {len(diagrams)} mermaid diagram(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
