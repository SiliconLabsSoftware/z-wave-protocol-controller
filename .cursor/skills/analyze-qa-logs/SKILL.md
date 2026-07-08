---
name: analyze-qa-logs
description: Two-stage QA log analysis that minimizes expensive model token usage. Stage 1 uses a cheap model to extract key info from raw logs into a markdown file. Stage 2 uses one or more powerful models to analyze the extract. Use when the user provides QA log files or directories and asks for log analysis, failure investigation, root cause analysis, or test result review.
disable-model-invocation: true
---

# QA Log Analysis (Two-Stage)

**Goal:** expensive models read a compact extract, not full raw logs.

1. **Extract** (cheap model): raw logs → structured `.md` file
2. **Analyze** (powerful model(s)): extract → conclusions (parallel when multiple models selected)

---

## Step 0: Collect inputs

Ask for log inputs if the user did not provide them.

### Log files

**Directory** — set `log-dir` and `<input-dirs>` to the absolute directory path. Build `<log-paths>` from:

- Present files: `test_error.log`, `test_warning.log`, `test_info.log`
- If `test_error.log` is absent, also add up to 3 other `.log` files (exclude `test_debug.log`, `zpc.log`, and files already in the list; smallest first)
- Add `test_debug.log` or `zpc.log` only if the user explicitly requests them

**Explicit file paths** — `<log-paths>` are exactly those files. Set `<input-dirs>` to their unique parent directories. Set `log-dir` to the shared parent, or the parent of the first file if paths span directories (for the default extract path only).

**If `<log-paths>` is empty, stop and tell the user. Do not launch Stage 1.**

### Extract path

Set `<extract-path>` before Stage 1:

- If the user's initial request names an output file (absolute path, or relative to `log-dir`), use it.
- Otherwise set `<extract-path>` to `<log-dir>/extract-YYYYMMDD-HHMMSS.md` using the current local time (fresh path each run).
- For any other custom path, ask for the full absolute path in chat before Stage 1.

### Models

Use `AskQuestion` (one call, two questions):

**Question 1 — Extractor** (id: `extractor_model`):
> Which model should parse the raw logs? (Stage 1 — cheaper is better)

- `composer-2.5` — Cheapest (Recommended)
- `composer-2.5-fast` — Faster, more expensive
- `claude-4.6-sonnet-medium-thinking` — Mid-tier
- `claude-sonnet-5-thinking-high` — Powerful

**Question 2 — Analyzer(s)** (id: `analyzer_model`, `allow_multiple: true`):
> Which model(s) should analyze the extract? (Stage 2 — at least one to run analysis; leave empty to extract only)

- `claude-4.6-sonnet-medium-thinking` — Mid-tier (Recommended)
- `claude-sonnet-5-thinking-high` — Powerful
- `claude-opus-4-8-thinking-high` — Deepest analysis
- `composer-2.5` — Cheap / quick

Record selected analyzer model slugs as `<analyzer-models>`. If empty, skip Stage 2 entirely.

---

## Step 1: Extract

Launch a Task subagent:

```
subagent_type: generalPurpose
model: <extractor-model>
run_in_background: false
```

Prompt (fill in `<log-paths>` and `<extract-path>`):

> You are a log extraction assistant. Your output is read by a separate analysis stage — **not** by a human scrolling raw logs. Optimize every section for downstream root-cause analysis: quantify, deduplicate, timestamp, name affected devices/nodes, and separate signal from noise.
>
> Read the log files below and write a structured markdown extract to `<extract-path>`. Be thorough but concise. Preserve ALL errors verbatim. Deduplicate repetitive lines (show count instead). For tracebacks, strip all framework/library frames (pytest, pluggy, etc.) and keep only lines from the actual test source file onwards.
>
> **Log files:** `<log-paths>`
>
> **Output structure:**
>
> ```markdown
> # QA Log Extract
>
> ## Executive summary
> <!-- 3–5 factual bullets for the analyzer: final result, what failed first, scope (devices/iterations), and whether failure looks isolated or systemic. No root-cause speculation. -->
> -
>
> ## Test metadata
> - Test name:
> - Date/time:
> - Hardware / firmware:
> - ZPC version / build (if logged):
> - Duration:
> - Log files analyzed:
>
> ## Result
> <!-- One-line outcome description, then PASS or FAIL -->
> PASS / FAIL
>
> ## First failure
> <!-- Earliest event that plausibly triggered the failure cascade. If PASS, write "N/A". -->
> - Timestamp:
> - Component / subsystem:
> - Device or node (if applicable):
> - Event:
> - Representative log line:
>
> ## Z-Wave metrics
> <!-- Count each occurrence across all log files. Use 0 if not found. -->
> - S2 timeouts:
> - S0 timeouts:
> - Failed transmissions (TX failures / no ACK from Z-Wave API):
> - Missing ACKs (ACK not received):
> - CRC / checksum errors:
> - Supervision report timeouts:
> - MQTT command timeouts:
> - SmartStart inclusion timeouts:
> - Inclusion / interview failures:
> - Exclusion / remove failures:
> - Reinclusion attempts (total):
> - Reinclusion successes:
> - Reinclusion failures:
> - Wake-up / sleeping-node keep-alive failures:
> - OTA transfer failures:
> - Devices non-functional at end:
>
> ## Device / node status
> <!-- One row per device or node mentioned in logs. Omit if not applicable. -->
> | Node / device | Role | Included | Interviewed | Functional at end | Notes |
> |---|---|---|---|---|---|
>
> ## MQTT failures
> <!-- Timeouts or errors on MQTT topics. Omit section if none. -->
> | Topic / command | Count | First timestamp | Last timestamp | Representative line |
> |---|---:|---|---|---|
>
> ## Error/warning signatures
> <!-- For each major error/warning pattern, one row. Quote at most one representative line. Sort by count descending. -->
> | Signature | Count | First timestamp | Last timestamp | Affected devices/components | Representative line |
> |---|---:|---|---|---|---|
> | SmartStart timeout waiting for Add/Report | | | | | |
>
> ## Iteration summary
> <!-- One row per iteration (or group of identical iterations). Omit if test is not iterative. -->
> | # | Devices | Included | Validated | Removed | Failures | Notes |
> |---|---|---|---|---|---|---|
> | 1 | ? | ? | ? | ? | ? | |
>
> ## Failure cascade
> <!-- Chronological list of significant failure events after the first failure. Factual only. Omit if PASS or single-point failure. -->
> 1.
>
> ## ZPC internal warnings breakdown
> <!-- Count per-iteration so accumulation vs setup-noise is visible. Omit columns with all zeros. -->
> | Warning | Iter 1 | Iter 2 | Iter 3 | Iter 4 | Iter 5+ |
> |---|---|---|---|---|---|
> | attribute_store deletion race | | | | | |
> | resolver child count changed  | | | | | |
> | zwave_tx frame drops          | | | | | |
>
> ## Errors
> <!-- All ERROR-level lines with timestamps. Repeated identical errors: show once + "(×N)" -->
>
> ## Warnings
>
> ### Relevant to failure
> <!-- WARNING-level lines plausibly related to the failure -->
>
> ### Setup / expected noise
> <!-- Known-benign warnings: NVM magic check, CC security level mismatches, zniffer parser errors, etc. Label each with why it is expected. -->
>
> ## Exceptions & tracebacks
> <!-- Strip pytest/pluggy/framework frames. Keep only lines from the actual test file onwards. -->
>
> ## Key events timeline
> <!-- Significant events in order: device power-on, flash erase, include, exclude, MQTT timeouts, interview steps, state changes, final status. Include timestamps. -->
>
> ## Analysis pointers
> <!-- Factual leads for the analyzer — ranked by relevance. Do NOT state root cause; list observable facts and where to look. -->
> 1.
>
> ## Gaps / not found in extract
> <!-- Information the analyzer may need but was absent from the provided log files. Suggest filenames (e.g. test_debug.log, zpc.log) if they would help. -->
> -
>
> ## Observations
> <!-- Patterns: which devices failed, which operations timed out, how many iterations succeeded before failure, correlation across metrics -->
> ```
>
> Reply with exactly: `EXTRACT_DONE: <extract-path>`

**On failure:** retry once if `EXTRACT_DONE` is missing or the file was not written. If it still fails, report `EXTRACT_FAILED` and stop.

**If `<analyzer-models>` is empty:** tell the user the extract is at `<extract-path>` and stop.

---

## Step 2: Analyze

**Run only when `<analyzer-models>` is non-empty.**

Launch one Task per analyzer model. Use `run_in_background: true` when multiple models are selected; `false` for a single model.

```
subagent_type: generalPurpose
model: <analyzer-model>
```

**First-pass prompt** (fill in `<extract-path>`):

> You are a QA engineer. Read only the extract at `<extract-path>`. Do not read raw log files on this pass.
>
> Start from **Executive summary**, **First failure**, **Z-Wave metrics**, **Device / node status**, **Failure cascade**, and **Analysis pointers**. Use **Gaps / not found in extract** to decide whether more logs are needed.
>
> Provide:
> 1. **Conclusion** — root cause (FAIL) or stability summary (PASS)
> 2. **Evidence** — quote sparingly from the extract (reference signatures, timeline, or metrics)
> 3. **Affected components** — devices, nodes, modules, or subsystems
> 4. **Recommended next steps** — concrete actions to investigate or fix
> 5. **Confidence** — High / Medium / Low with one-line justification
>
> If the extract lacks detail for at least Medium confidence, respond with exactly:
> ```
> NEED_MORE_DETAIL
> Missing: <what is needed>
> Suggested files: <comma-separated filenames, e.g. test_debug.log, zpc.log>
> ```
> Do not return a Low-confidence analysis instead of this stub.

### Handle responses

For each analyzer:

1. **Usable analysis** (has conclusion + confidence, not a stub) → keep as final result.
2. **`NEED_MORE_DETAIL` stub** → collect for user approval (see below).
3. **Empty, timeout, or malformed** → retry once with the first-pass prompt. If still bad, mark `ANALYSIS_FAILED`.

**When any analyzer returned `NEED_MORE_DETAIL`:**

- Show what is missing and which files were suggested (union across all stubs).
- `AskQuestion`: _"Allow analyzers to read additional raw log files?"_
  - `Yes` / `No`
- Resolve suggested filenames by searching each directory in `<input-dirs>`; use a unique match, ask the user if ambiguous, skip if not found (tell the user).
- Re-launch stub-requesting analyzers:
  - **Yes** with at least one resolved file: read `<extract-path>` plus those raw logs. Full analysis; do not return `NEED_MORE_DETAIL`.
  - **No**, or **Yes** with no resolved files: read only `<extract-path>`. Best-effort analysis; confidence must be Low.
- If a re-launched analyzer is still unusable, retry once with the **same** re-launch instructions (Yes or No path as used). Do not use the first-pass prompt. If it still fails or returns `NEED_MORE_DETAIL`, mark `ANALYSIS_FAILED`.

If every analyzer ends in `ANALYSIS_FAILED`, report the blocker and stop.

---

## Step 3: Present

Tell the user the extract is saved at `<extract-path>`.

**One analyzer:** return its analysis.

**Multiple analyzers:** one section per model:

```
## Analysis — <model-slug>
<content>
```

Add a short **Comparison** (3–5 sentences): where models agree, diverge, and which conclusion is best supported. Note if some models had raw-log access and others did not.

---

## Model reference

| Model slug | Tier | Best for |
|---|---|---|
| `composer-2.5` | cheapest | Stage 1 default |
| `composer-2.5-fast` | faster | Stage 1 when speed matters |
| `claude-4.6-sonnet-medium-thinking` | mid | Stage 2 lightweight |
| `claude-sonnet-5-thinking-high` | powerful | Stage 2 default |
| `claude-opus-4-8-thinking-high` | most powerful | Stage 2 deep analysis |
