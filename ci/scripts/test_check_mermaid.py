#!/usr/bin/env python3
"""Unit tests for ci/scripts/check_mermaid.py policy rules."""
from __future__ import annotations

import importlib.util
import sys
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).resolve().parent / "check_mermaid.py"
_spec = importlib.util.spec_from_file_location("check_mermaid", _SCRIPT)
check_mermaid = importlib.util.module_from_spec(_spec)
assert _spec.loader is not None
_spec.loader.exec_module(check_mermaid)

banned_issues = check_mermaid.banned_issues


class TestBannedMermaidPolicy(unittest.TestCase):
    def test_allows_fixed_inclusion_style(self) -> None:
        source = """sequenceDiagram
    alt Standard S2 key grant (not SmartStart)
        A->>B: test
    end
    NM-->>NM: return to NM_IDLE
"""
        self.assertEqual(banned_issues(source), [])

    def test_rejects_unicode_arrow_in_self_message(self) -> None:
        source = "sequenceDiagram\n    NM-->>NM: → NM_IDLE\n"
        issues = banned_issues(source)
        self.assertTrue(any("Unicode arrows" in i for i in issues))

    def test_rejects_unicode_arrow_in_state_label(self) -> None:
        source = "sequenceDiagram\n    NM-->>NM: NM_IDLE → NM_WAITING_FOR_ADD\n"
        issues = banned_issues(source)
        self.assertTrue(any("Unicode arrows" in i for i in issues))

    def test_rejects_unquoted_colon_in_alt(self) -> None:
        source = "sequenceDiagram\n    alt S2: Keys requested\n    A->>B: x\n    end\n"
        issues = banned_issues(source)
        self.assertTrue(any("alt/opt/par" in i for i in issues))

    def test_allows_quoted_alt_with_colon(self) -> None:
        source = 'sequenceDiagram\n    alt "S2: Keys requested"\n    A->>B: x\n    end\n'
        self.assertEqual(banned_issues(source), [])

    def test_rejects_html_br(self) -> None:
        source = "graph TD\n    A[foo<br/>bar]\n"
        issues = banned_issues(source)
        self.assertTrue(any("HTML tags" in i for i in issues))

    def test_allows_unicode_arrow_inside_quoted_state_label(self) -> None:
        source = 'stateDiagram-v2\n    state "Caps→Set→Get" as WAKE_UP\n'
        self.assertEqual(banned_issues(source), [])


if __name__ == "__main__":
    unittest.main()
