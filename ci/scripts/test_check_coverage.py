"""Regression checks for native LLVM collection and component coverage gates."""
import argparse
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

import check_coverage as gate


def llvm_report(files):
    return {"type": "llvm.coverage.json.export", "data": [{"files": files}]}


def covered_file(lines=(10, 10), branches=(2, 2)):
    return {"filename": "components/example/src/example.cpp", "summary": {
        "lines": {"covered": lines[0], "count": lines[1]},
        "branches": {"covered": branches[0], "count": branches[1]},
    }}


class CollectionTests(unittest.TestCase):
    def test_merges_enabled_tests_and_uses_native_llvm_for_json_lcov_and_html(self):
        with TemporaryDirectory() as directory:
            build = Path(directory)
            (build / "CMakeCache.txt").write_text("CMAKE_CXX_COMPILER:FILEPATH=/tools/clang++\nCOVERAGE:BOOL=ON\n")
            binary = build / "test_example"
            binary.touch()
            profiles = build / "coverage/profiles/test_example"
            profiles.mkdir(parents=True)
            profile = profiles / "run.profraw"
            profile.touch()
            test = {"command": [str(binary)], "properties": [
                {"name": "ENVIRONMENT", "value": ["LLVM_PROFILE_FILE=profile"]},
            ]}
            disabled = {"command": [str(build / "disabled")], "properties": test["properties"] + [
                {"name": "DISABLED", "value": True},
            ]}
            metadata = {"tests": [test, test, disabled, {"command": ["python3"]}]}
            mapped = llvm_report([covered_file()])
            with patch.object(gate.subprocess, "check_output", side_effect=[json.dumps(metadata), json.dumps(mapped)]), patch.object(
                gate.subprocess, "run"
            ) as run, patch.object(gate.shutil, "which", side_effect=lambda name: name), patch.dict(
                gate.os.environ, {"LLVM_PROFDATA": "/tools/llvm-profdata"}
            ):
                result = gate.run_llvm_coverage(build, build / "coverage")
            self.assertEqual(result, build / "coverage/coverage.json")
            merge, export, lcov, html = [call.args[0] for call in run.call_args_list]
            self.assertEqual(merge[:3], ["/tools/llvm-profdata", "merge", "-sparse"])
            self.assertEqual(merge.count(str(profile)), 1)
            self.assertEqual(export[:2], ["/tools/llvm-cov", "export"])
            self.assertIn("-summary-only", export)
            self.assertEqual(lcov[:2], ["/tools/llvm-cov", "export"])
            self.assertIn("-format=lcov", lcov)
            self.assertIn(str(binary), lcov)
            self.assertEqual(html[:2], ["/tools/llvm-cov", "show"])
            self.assertIn("-format=html", html)
            self.assertIn("-show-branches=count", html)
            self.assertIn(str(binary), export)
            self.assertIn("--sources", export)
            self.assertTrue(all(call.kwargs["check"] for call in run.call_args_list))

    def test_missing_or_stale_profiles_fail_before_reporting(self):
        with TemporaryDirectory() as directory:
            build = Path(directory)
            (build / "CMakeCache.txt").write_text("CMAKE_CXX_COMPILER:FILEPATH=/tools/clang++\nCOVERAGE:BOOL=ON\n")
            binary = build / "test_example"
            binary.touch()
            profiles = build / "coverage/profiles/test_example"
            profiles.mkdir(parents=True)
            metadata = {"tests": [{"command": [str(binary)], "properties": [
                {"name": "ENVIRONMENT", "value": ["LLVM_PROFILE_FILE=profile"]},
            ]}]}
            for stale in (False, True):
                if stale:
                    profile = profiles / "old.profraw"
                    profile.touch()
                    gate.os.utime(profile, ns=(1, 1))
                with self.subTest(stale=stale), patch.object(gate.shutil, "which", return_value="tool"), patch.object(
                    gate.subprocess, "check_output", return_value=json.dumps(metadata)
                ), patch.object(gate.subprocess, "run") as run, self.assertRaisesRegex(SystemExit, "no current profiles"):
                    gate.run_llvm_coverage(build, build / "coverage")
                run.assert_not_called()


class GateTests(unittest.TestCase):
    def run_gate(self, files):
        with TemporaryDirectory() as directory:
            root = Path(directory)
            component = root / "components/example"
            (component / "test").mkdir(parents=True)
            (component / "CMakeLists.txt").touch()
            (root / "blacklist.yaml").write_text("blacklist: []\n")
            (root / "thresholds.yaml").write_text("min_line_percent: 90\nmin_branch_percent: 90\n")
            (root / "build/coverage").mkdir(parents=True)
            (root / "build/coverage/coverage.json").write_text(json.dumps(llvm_report(files)))
            args = argparse.Namespace(build_dir=Path("build"), blacklist=Path("blacklist.yaml"), thresholds=Path("thresholds.yaml"), skip_report=True)
            output = StringIO()
            with patch.dict(gate.os.environ, {"GITHUB_STEP_SUMMARY": ""}), patch.object(
                gate, "REPO_ROOT", root
            ), patch.object(gate, "parse_args", return_value=args), redirect_stdout(output), redirect_stderr(output):
                result = gate.main()
            return result, output.getvalue()

    def test_gated_component_with_no_data_fails(self):
        result, output = self.run_gate([])
        self.assertEqual(result, 1)
        self.assertIn("no executable line coverage data", output)
        self.assertIn("❌ No data", output)

    def test_straight_line_component_needs_no_artificial_branches(self):
        result, _ = self.run_gate([covered_file(branches=(0, 0))])
        self.assertEqual(result, 0)

    def test_uncovered_branch_fails_even_when_all_lines_execute(self):
        result, output = self.run_gate([covered_file(branches=(1, 2))])
        self.assertEqual(result, 1)
        self.assertIn("branch 50.0%", output)

    def test_uncovered_lines_fail_even_when_all_branches_execute(self):
        result, output = self.run_gate([covered_file(lines=(8, 10))])
        self.assertEqual(result, 1)
        self.assertIn("line 80.0%", output)

    def test_generated_and_test_files_do_not_change_the_component_totals(self):
        generated = covered_file(lines=(0, 100), branches=(0, 100))
        generated["filename"] = "components/example/generated/code.cpp"
        test = {**generated, "filename": "components/example/test/test.cpp"}
        result, output = self.run_gate([covered_file(), generated, test])
        self.assertEqual(result, 0)
        self.assertIn("100.0% (10/10)", output)


if __name__ == "__main__":
    unittest.main()
