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

## Required completion checklist

Before considering a test-writing task complete, verify every item below:

- [ ] Every `TEST` / `TEST_F` has a short comment immediately above it describing
      the behavior the test proves.
- [ ] Every test has the applicable `CC:` requirement ID from the specification.
      If the specification defines no requirement for that behavior, omit the
      ID rather than guessing one.
- [ ] Comments explain the meaningful setup, action, and assertion blocks inside
      the test. Explain non-obvious choices; do not add comments that merely
      restate the next line.
- [ ] Fixture maps explain each field at the point of use. Wire-frame fixtures
      and assertions explain the byte order and the meaning of each non-obvious
      value, including why the selected version and values apply.
- [ ] There are no unexplained numeric literals. Give protocol values, expected
      counts, lengths, offsets, fixture sizes, and byte positions named constants;
      a literal may remain only when it is immediately explained at its use.
- [ ] Command Class versions with different behavior have separate, identifiable
      tests, and success, rejection, fallback, and storage behaviors are atomic.
- [ ] Review the full component scope: list the public handlers, supported
      Command Class versions, and meaningful success, rejection, fallback, and
      error paths. Map each to a test or explain why it is not testable.
- [ ] For important validation, capacity, optional-field, and state-lookup
      conditions, test both outcomes where meaningful. Do not invent invalid
      inputs solely to reach defensive branches that cannot occur in valid use.
- [ ] Verify that each test would fail if the behavior it covers were removed
      or reversed. Tests must assert the observable result, not only that a call
      returned success.
- [ ] Verify test isolation: each test resets shared mocks, connector state,
      attribute-store state, and process-global state so outcomes do not depend
      on test execution order.
- [ ] Review the complete diff and confirm that only intended test, test-support,
      build-registration, and coverage-policy files changed. Get explicit user
      authorization before changing production source.
- [ ] After the final edit, rebuild and run the full sanitizer suite, then run
      the full coverage suite and coverage gate. Focused test runs and reports
      generated before the last edit do not satisfy this check.
- [ ] If a coverage gate fails, report the component's line and branch counts
      and the specific uncovered paths. Explain why meaningful tests cannot
      exercise them and consult the user about authorized next steps.
- [ ] `clang-format` and `git diff --check` pass for the changed tests.

Do this comment-and-requirement audit explicitly after writing the tests. Do not
assume that descriptive test names or comments above the fixture satisfy these
items. If a required `CC:` ID is unclear, check the specification before
omitting it.

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
- **Isolate the unit under test from collaborators:** replace collaborators at their public boundary when their implementation is outside the unit's scope. Use a fake for simple stateful responses and GoogleMock (`MOCK_METHOD` / `EXPECT_CALL`) when interactions matter. Link the implementation under test without constructing or linking real collaborator implementations; this keeps tests focused and lets them control collaborator responses. For example, when testing one Command Class (CC) that queries or notifies another over `component_connector`, bind a mock to the peer CC's typed connector events.
- Keep a reusable collaborator fake/mock in a separate test-support header/source when it has boundary bindings, default behavior, or synchronization helpers. Configure ordinary responses with simple defaults; use focused interaction expectations only where the interaction itself is under test. Add support files to the test target's `SRCS` so they are visible in the build definition.
- Await completion at asynchronous boundaries. For request/response calls, wait on the returned future and assert status and result. For fire-and-forget notifications, synchronize on the fake/mock callback before inspecting its state. Avoid sleeps and unrelated operations used only as synchronization fences.
- Keep collaborator state separate from the unit's own state. For Command Class state, use the RAM attribute store and reserve the fake/mock for collaborator behavior. Reset expectations and configurable responses between tests, especially when a process-wide fake/mock is used.
- Tests must be deterministic: no `sleep` for sync, no real network/time, unique temp paths, reset process-global state in the fixture.
- Add verbose comments to the test cases to be easier to follow what and why it is going on. Comment each block inside the test cases.
- Explain fixture maps and wire-frame bytes at the point of use. Comments should identify what each field or byte represents and why the selected value is used; a general comment above a fixture is not a substitute for field-level explanations.
- In case of command classes, create separated test functions for each version tests to have more atomic test cases.
- Keep command behavior cases atomic: use one test for each distinct success, rejection, fallback, or storage behavior rather than combining multiple unrelated outcomes. Shared setup helpers are fine, but assertions should make each behavior independently identifiable by the test name and failure output.
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
- Require at least **90% line and 90% branch coverage** for every tested
  component. Both gates must pass before the work is complete.
- Confirm the child’s own row in the `check_coverage.py` table is `pass` for both line and branch coverage, not rolled into a blacklisted parent.
- Missing executable line data for a gated component is a failure. Zero branches is valid for straight-line code.
- Install Python dependencies from `ci/dependencies/requirements.txt` (no additional Python coverage package is needed). Run the reporting script with that Python environment.
- The script uses `llvm-profdata` and `llvm-cov` beside the configured compiler. Set `LLVM_PROFDATA` for a versioned/custom toolchain, e.g. `llvm-profdata-17`; the script uses the corresponding `llvm-cov` name.
- Configuring coverage clears previous coverage output. After code changes, rebuild and run all coverage tests before reporting; use `--fresh` for a clean baseline.
- Branches are source condition outcomes; no throw/unreachable branch filters are applied. Still test meaningful error and exception behavior explicitly. Exact counts can differ between OS targets and compiler versions.
- Reports: `build/unit-test-coverage/coverage/html/index.html`, `build/unit-test-coverage/coverage/coverage.json`, and `build/unit-test-coverage/coverage/lcov.info` (for editor coverage plugins).
- Do not finish if `check_coverage.py` fails. Add focused tests for meaningful
  paths or correct a component-path mistake; do not lower the threshold or
  re-blacklist a tested component to silence the gate.
- If valid tests cannot reach 90% line or branch coverage, report the component's
  counts and specific uncovered paths, explain why valid tests cannot exercise
  them, then consult the user about next steps before changing source or
  coverage policy. Options include an authorized testability change or an
  explicit component-owner-approved exemption through repository policy.

## CI

PR `unit-test` job: sanitizer gcc → sanitizer clang → clang `unit-test-coverage` → `check_coverage.py` (minimums from `ci/unit_test_config.yaml`) → `unit-test-coverage` artifact (HTML, native JSON, and LCOV) and coverage table in the job summary.
