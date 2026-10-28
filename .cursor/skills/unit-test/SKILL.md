---
name: unit-test
description: >-
  Write or update ZPC GoogleTest/GoogleMock component tests. Unblacklist the
  component first, then TDD, sanitizer unit-test preset, and coverage gate.
  Use when adding tests, diagnosing unit-test or coverage failures, or changing
  ci/coverage/component_blacklist.yaml.
---

# ZPC component unit tests (TDD)

Follow this sequence in order. The skill is not done until the coverage check passes.

## Failures and specification

- **Specification injection:** check whether the relevant Z-Wave specification is in the workspace or repo. If it is not, ask the user where it is. The previous specification matters. Tell the user which specification and version you found.
- The specification is authoritative for expected behavior. Do not infer expected behavior from the implementation or change a test merely to make it pass.
- For each failing test, inspect the individual assertion and compare expected and actual behavior with the applicable specification requirement:
  - If the test expectation conflicts with the specification, correct the test and cite the relevant requirement.
  - If the implementation conflicts with the specification, treat it as a source defect and preserve a test for the specified behavior. Summarize the evidence and follow the user's authorization before changing source.
  - Report remaining failures one by one, including the expected behavior, actual behavior, and whether the test or implementation is inconsistent with the specification.
- In case of command classes there are multiple versions of each. Create test cases for each versions, based on the specification.
- **Command Class version compatibility (CC:0000.00.00.11.002):** for an existing Command ID, outgoing controller tests MUST assert the controller's implemented wire format, including appended fields, even when the peer advertises an older version. The older receiver MUST NOT discard the frame because of its length; it interprets the portion defined by its version. Do not shorten a Set/Get solely because the peer reports v1 or v3. Use values that are valid and meaningful in the older version's prefix; do not send a value introduced in a later version as though the older peer supported it. Gate a command only when its Command ID does not exist in the peer's version. For incoming commands, test both the older shorter frame and the current-version frame, confirming that the supported older prefix is parsed.
- If a requirement number exists, put it on the test (`CC:` IDs such as `CC:008A.03.02.11.008`). Skip the ID only when the spec has none for that behavior.
- Add a short description for each test case what do they test exactly

Sanitizers (`unit-test`) and coverage (`unit-test-coverage`) are **separate configures**. CMake fatals if `ZPC_ENABLE_SANITIZERS` and `COVERAGE` are both ON. Coverage uses Clang native LLVM profiles on macOS and in CI, read directly by LLVM’s own tools. CI keeps both GCC and Clang sanitizer runs, followed by Clang coverage. HTML, native JSON, and LCOV upload as artifact `unit-test-coverage`.

## Step 0 — unblacklist the component

Coverage uses a **blacklist only** ([`ci/coverage/component_blacklist.yaml`](ci/coverage/component_blacklist.yaml)). Do not add a whitelist or edit `GATED_ALWAYS` / extra allowlists in [`ci/scripts/check_coverage.py`](ci/scripts/check_coverage.py).

Identify the directory under test (same prefix as the YAML).

- **Existing module:** delete that path from the YAML **before writing tests**.
- **Path missing from the YAML:** stop. For a *new* untested module, `generate-new-cc` (or the owner) must **append** the path first. Then come back and remove it here. Except if the test is already exist and you are updating it, that case you have nothing to do with the yaml file. Only check it if the test is not accidently blacklisted.
- A **parent** prefix may stay listed (e.g. `components/command_classes` for shared `src/`). Delete only the child being tested (`components/command_classes/command_class_<name>`).
- After `test/` exists, `check_coverage.py` gates that child on its own path even if the parent remains blacklisted. Untested siblings stay under the parent exemption.
- Do not leave the tested component on the blacklist after this skill runs.

## Tests

Copy existing unit tests. Do not invent a new layout or fixture style.

### Attribute store in tests

Do not change `attribute_store.h` or the store implementation.

- **Attribute store is not under test** (command classes, network manager, …): use the RAM tree. Call `attribute_store_init()` / `attribute_store_teardown()` only. Do **not** call `datastore_init`. Do not mock `emplace_node` / `set_desired`. Assert children and desired/reported after Set/Get.
- **Attribute store is under test**: then you may use the real datastore; that is the only case for SQLite in unit tests.

### Conventions

- Layout: `components/<name>/test/` (command classes: `components/command_classes/command_class_<name>/test/`).
- Register from the component `CMakeLists.txt`: `if(UNIT_TEST) add_subdirectory(test) endif()`.
- Test `CMakeLists.txt`: `add_zpc_gtest` in [`cmake/include/gtest_test.cmake`](cmake/include/gtest_test.cmake) (`GTest::gtest_main`, `GTest::gmock`).
- License header: match [`components/crc16_ccitt/test/test_zwave_crc16.cpp`](components/crc16_ccitt/test/test_zwave_crc16.cpp).
- TDD: RED (failing test that names the spec behavior, with `CC:` when present) → GREEN (smallest change) → REFACTOR (tests stay green).
- `TEST` for independent cases; `TEST_F` when `SetUp`/`TearDown` is needed. `ASSERT_*` for preconditions, `EXPECT_*` for checks that should all run.
- Prefer fakes for state; gMock (`MOCK_METHOD` + `EXPECT_CALL`) for interaction seams (MQTT / TX), not the attribute-store tree.
- Tests must be deterministic: no `sleep` for sync, no real network/time, unique temp paths, reset process-global state in the fixture.
- Add verbose comments to the test cases to be easier to follow what and why it is going on. Comment each block inside the test cases.
- Explain fixture maps and wire-frame bytes at the point of use. Comments should identify what each field or byte represents and why the selected value is used; a general comment above a fixture is not a substitute for field-level explanations.
- In case of command classes, create separated test functions for each version tests to have more atomic test cases.
- Use meaningful test case names that describe the behavior and reflect the Command Class version when relevant.
- Do not use unexplained magic numbers. Define named constants for protocol values, expected counts, lengths, offsets, and fixture sizes; if a literal is clearer, add an inline comment explaining its meaning.
- Shared test helpers may reduce duplication, but must accept version-specific inputs or expectations when the specification defines different valid values. Keep each version's assertion independently identifiable in test output.
- Run `clang-format` on changed test files and verify formatting before finishing.

## Sanitizer run (correctness)

Never pass `COVERAGE=ON` here.

```bash
cmake --workflow --preset unit-test --fresh
# re-run after a successful configure/build:
ctest --preset unit-test
```

ASan/UBSan abort on error (`ASAN_OPTIONS` / `UBSAN_OPTIONS` in the `unit-test` test preset).

## Last step — coverage gate

```bash
cmake --workflow --preset unit-test-coverage --fresh
python3 ci/scripts/check_coverage.py --build-dir build/unit-test-coverage
```

- [`ci/coverage/component_blacklist.yaml`](ci/coverage/component_blacklist.yaml) is the only exemption list. Coverage minimums come from [`ci/unit_test_config.yaml`](ci/unit_test_config.yaml). A tested component (`CMakeLists.txt` + `test/`) omitted from the blacklist must meet both configured minimums for line and branch coverage.
- Confirm the child’s own row in the `check_coverage.py` table is `pass` for both line and branch coverage, not rolled into a blacklisted parent.
- Missing executable line data for a gated component is a failure. Zero branches is valid for straight-line code.
- Install Python dependencies from `ci/dependencies/requirements.txt` (no additional Python coverage package is needed). Run the reporting script with that Python environment.
- The script uses `llvm-profdata` and `llvm-cov` beside the configured compiler. Set `LLVM_PROFDATA` for a versioned/custom toolchain, e.g. `llvm-profdata-17`; the script uses the corresponding `llvm-cov` name.
- Configuring coverage clears previous coverage output. After code changes, rebuild and run all coverage tests before reporting; use `--fresh` for a clean baseline.
- Branches are source condition outcomes; no throw/unreachable branch filters are applied. Still test meaningful error and exception behavior explicitly. Exact counts can differ between OS targets and compiler versions.
- Reports: `build/unit-test-coverage/coverage/html/index.html`, `build/unit-test-coverage/coverage/coverage.json`, and `build/unit-test-coverage/coverage/lcov.info` (for editor coverage plugins).
- Do not finish if `check_coverage.py` fails. Add tests or fix the path prefix; do not re-add the component to the blacklist to silence the gate.

## CI

PR `unit-test` job: sanitizer gcc → sanitizer clang → clang `unit-test-coverage` → `check_coverage.py` (minimums from `ci/unit_test_config.yaml`) → `unit-test-coverage` artifact (HTML, native JSON, and LCOV) and coverage table in the job summary.
