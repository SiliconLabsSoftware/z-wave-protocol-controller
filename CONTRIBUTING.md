# Contributing to Z-Wave Protocol Controller (ZPC)

Thank you for your interest in contributing to the Z-Wave Protocol Controller project! This guide will help you understand our contribution process and ensure your contributions align with our standards.

## Table of Contents

- [Contribution Workflow](#contribution-workflow)
  - [GitHub Issues (Required)](#github-issues-required)
  - [Branch Naming](#branch-naming)
  - [Commit Messages](#commit-messages)
  - [Pull Request Process](#pull-request-process)
- [Code Style Guidelines](#code-style-guidelines)
- [Testing](#testing)
- [Documentation](#documentation)
- [Additional Guidelines](#additional-guidelines)

## Contribution Workflow

### GitHub Issues (Required)

**Every feature and bug fix must have a corresponding GitHub issue before work begins.**

#### Why Issues Are Required

- **Tracking**: Issues provide a clear record of what work is being done and why
- **Discussion**: Issues allow for discussion and refinement of ideas before implementation
- **Approval**: Issues help maintainers understand and approve the direction of changes
- **Context**: Issues provide context for reviewers when examining pull requests

#### Creating Issues

1. **Bug Reports**: Use the [Bug Report template](.github/ISSUE_TEMPLATE/bug.yaml) when creating a bug report. The template will guide you through providing all necessary information.

2. **Feature Requests**: Use the [Feature Request template](.github/ISSUE_TEMPLATE/feature.yaml) when proposing a feature. The template will guide you through providing all necessary information.

3. **Check for Existing Issues**: Before creating a new issue, search existing issues to avoid duplicates.

### Branch Naming

> [!NOTE]
> All variations of the `GH` prefix are acceptable.

**Branch names must start with `gh-<issue_number>` or `<issue_number>`**

#### Format

- **Preferred**: `gh-<issue_number>` (e.g., `gh-123`, `gh-456`)
- **Also Acceptable**: `<issue_number>` (e.g., `123`, `456`)

#### Examples

```bash
# Preferred format
gh-123-fix-memory-leak
gh-456-add-new-command-class

# Also acceptable (especially when created from GitHub issue)
123-fix-memory-leak
456-add-new-command-class
```

**Note**: GitHub automatically creates branches with just the issue number when creating a branch from an issue, which is acceptable. However, the `gh-` prefix is preferred for consistency.

### Commit Messages

**Commit messages must start with `GH-<issue_number>` or `#<issue_number>`**

#### Format

```
<prefix><issue_number>: <descriptive message>

[Optional: Additional details in commit body]
```

#### Examples

```bash
# Using GH- prefix
GH-123: Fix memory leak in network manager
GH-456: Add support for new command class

# Using # prefix
#123: Fix memory leak in network manager
#456: Add support for new command class
```

#### Best Practices

- Use clear, descriptive commit messages
- Keep the first line under 72 characters when possible
- Reference the issue number in the commit message
- Optionally include additional context in the commit body
- Use present tense ("Fix bug" not "Fixed bug")
- Be specific about what changed and why

#### Example with Body

```
GH-123: Fix memory leak in network manager

The network manager was not properly releasing memory when nodes
were removed from the network. This change ensures all allocated
resources are freed during node deletion.

Fixes #123
```

### Pull Request Process

1. **Create Pull Request**
   - Ensure your branch is up to date with the main branch
   - Push your branch to your fork
   - Create a pull request on GitHub

2. **PR Description Requirements**
   - Reference the GitHub issue in the PR description (e.g., "Fixes #123" or "Closes #456")
   - Use the [pull request template](.github/pull_request_template.md)
   - Describe what changes were made and why
   - Include any relevant testing information

3. **Checklist Completion**
   - Complete all items in the PR template checklist
   - Ensure a [Contribution License Agreement (CLA)](https://en.wikipedia.org/wiki/Contributor_License_Agreement) has been established between Silicon Labs and your company (matching email domain)

4. **Code Review**
   - All PRs require review and approval from maintainers
   - Address review comments promptly
   - Keep discussions constructive and professional

5. **CI/CD Checks**
   - All CI/CD checks must pass before a PR can be merged
   - PR CI runs the following gates in order:
     1. **clang-format** — fails if formatting drift is detected (`--dry-run --Werror`)
     2. **clang-tidy**, **Linux build** (`amd64-gcc`, `arm64-gcc`, `amd64-clang`), and **macOS build** — run in parallel on native runners after format passes

6. **Merge**
   - Once approved and all checks pass, a maintainer will merge your PR
   - Your contribution will be included in the next release

## Code Style Guidelines

### Formatting

This project uses **LLVM 19** `clang-format`. The configuration is defined in [`.clang-format`](.clang-format).

**Toolchain (align on LLVM major 19; patch level need not match CI exactly):**

| Platform | `clang-format` binary |
|----------|----------------------|
| **macOS (Homebrew `llvm@19`)** | `$(brew --prefix llvm@19)/bin/clang-format` |
| **Linux / CI (Ubuntu)** | `clang-format-19` |

Do **not** put `llvm@19` on your global `PATH` — invoke the binary by full path (macOS) or the versioned apt name (Linux). On Ubuntu, unversioned `clang-format` is LLVM 18 and must not be used.

**Do not bulk-format generated code.** C/C++ under `components/**/generated/` is produced by the command-class generator at configure time (see [`.gitignore`](.gitignore)). Only format hand-written sources. Paths excluded from formatting are listed in [`.clang-format-ignore`](.clang-format-ignore).

**Format or verify all hand-written C/C++**:

```bash
./ci/scripts/clang-format.sh format   # apply formatting
./ci/scripts/clang-format.sh check    # verify (same check CI runs)
```

The script resolves the LLVM 19 binary per platform. Override with `CLANG_FORMAT` if needed.

**Test staged files before commit** (no changes applied):

```bash
./ci/scripts/clang-format.sh check --staged
```

**Fix staged formatting** (same logic the pre-commit hook uses):

```bash
./ci/scripts/clang-format.sh format --staged
```

You can also use the VS Code tasks **"Format all code (clang-format)"** or **"Check code format (clang-format)"** from the command palette (`Tasks: Run Task`).

Many editors can be configured to format on save using clang-format.

#### Local pre-commit checks

Optionally install the repository pre-commit hook (formats staged C/C++ via the same script):

```bash
./.githooks/install.sh
```

This sets `core.hooksPath` to `.githooks` for your local clone only. On commit, the hook runs `./ci/scripts/clang-format.sh format --staged` and re-stages fixed files. Run `./ci/scripts/clang-format.sh check --staged` before `git commit` to catch formatting issues early.

#### CI enforcement

The CI lint job runs `./ci/scripts/clang-format.sh check` with the same [`.clang-format-ignore`](.clang-format-ignore) exclusions. Fix locally and push — CI does not auto-commit format fixes.

### clang-tidy

This project uses **LLVM 19** `clang-tidy`. The configuration is defined in [`.clang-tidy`](.clang-tidy).

**Toolchain (install via dependency files — do not use one-off package installs):**

| Platform | Dependency file | Binary |
|----------|-----------------|--------|
| **macOS (Homebrew `llvm@19`)** | [`ci/dependencies/brew-packages.txt`](ci/dependencies/brew-packages.txt) | `$(brew --prefix llvm@19)/bin/clang-tidy` |
| **Linux / CI (Debian)** | [`ci/dependencies/apt-packages-base.txt`](ci/dependencies/apt-packages-base.txt) + [`ci/dependencies/apt-packages-clang-tidy.txt`](ci/dependencies/apt-packages-clang-tidy.txt) | `clang-tidy-19` |

Install dependencies with the same commands as [getting-started](docs/getting-started.md):

```bash
brew install $(cat ci/dependencies/brew-packages.txt)          # macOS
apt-get install -y $(cat \
  ci/dependencies/apt-packages-base.txt \
  ci/dependencies/apt-packages-clang-tidy.txt)     # Linux
```

**Run the same check as CI** (full `applications/` and `components/` tree):

```bash
./ci/scripts/clang-tidy.sh check
```

The script resolves the LLVM 19 binary per platform. Override with `CLANG_TIDY` or `RUN_CLANG_TIDY` if needed. You can also use the VS Code task **"Run clang-tidy"** from the command palette (`Tasks: Run Task`). This check is intentionally **not** wired into git hooks — a full-tree tidy run is too slow for commit-time feedback.

#### CI enforcement

The CI clang-tidy job runs `./ci/scripts/clang-tidy.sh check` on a native Ubuntu runner. Fix locally and push — CI does not auto-fix tidy findings.

### C++ Coding Standards

- **New Modules**: Must be implemented in C++. If the new component needs to be used in C modules, a C wrapper must be created.
- **Component Coupling**: Components should be loosely coupled through the `component_connector` component (previously named `command_class_connector`).
- **Code Organization**: Follow the existing project structure and organization patterns.

### General Guidelines

- Write clear, self-documenting code
- Use meaningful variable and function names
- Add comments for complex logic or non-obvious behavior
- Follow existing code patterns and conventions
- Keep functions focused and reasonably sized
- Avoid code duplication

## Testing

### Prerequisites

Before building or testing, install the platform dependencies. See [docs/getting-started.md](docs/getting-started.md) for the full list of tested compilers, dependency files (`ci/dependencies/`), and Python virtual environment guidance.

### Build Verification

Ensure the project builds on your target platform:

```bash
cmake --workflow --preset macos   # on macOS
cmake --workflow --preset debian  # on Linux / inside Docker
```

The configure step (`cmake --preset <preset>`) also **regenerates all command class code** by invoking the command class generator; keep this in mind when adding or modifying command classes.

### CI Expectations

PR CI runs these gates automatically — you do not need to replicate all of them locally:

1. **clang-format** — formatting is checked first; drift fails the format job
2. **clang-tidy**, **Linux builds** (Debian packages for `amd64` and `arm64`), and **macOS build** — run in parallel after format passes

## Documentation

### Documentation Requirements

When adding new features or modifying existing functionality:

1. **Update Relevant Documentation**
   - Update API documentation if interfaces change
   - Update user-facing documentation if behavior changes
   - Update developer documentation if architecture changes

2. **Documentation Locations**
   - Component documentation (source files): `components/<component_name>/doc/` or `components/<component_name>/docs/`
   - MQTT API documentation (source files): `components/mqtt_api/doc/`
   - Command class documentation (source files): `components/command_classes/<command_class>/doc/`
   - Main project documentation (source files): root markdown files and `docs/`
   - Generated/merged MkDocs pages: `docs/api/` (do not edit manually; regenerated by script)

3. **MkDocs Publishing Workflow**

   To build docs locally with full parity to CI:

   ```bash
   pip install -r scripts/mkdocs/requirements.txt
   pip install -r scripts/command_class_generator/requirements.txt

   mkdir -p build/command_class_docs
   python3 scripts/command_class_generator/main.py \
     --xml scripts/command_class_generator/zwave.xml \
     --xsd scripts/command_class_generator/zwave.xsd \
     --cfg scripts/command_class_generator/config.yaml \
     --templates scripts/command_class_generator/templates \
     --out build/command_class_docs

   GENERATED_CC_DOCS=build/command_class_docs \
     python3 scripts/mkdocs/prepare_docs_for_mkdocs.py

   mkdocs build --strict
   # optional: mkdocs serve
   ```

   - The canonical command class implementation guide is [docs/command_class_implementation_guide.md](docs/command_class_implementation_guide.md) — edit it directly there.
   - `docs/api/` is generated by `prepare_docs_for_mkdocs.py`; do not edit it manually.

4. **MQTT API Documentation**
   - If adding or modifying MQTT API endpoints, update the relevant MQTT interface documentation
   - Start from the [MQTT API index](components/mqtt_api/doc/mqtt_api_index.md) and, when adding a new API class, keep [MQTT API Overview](components/mqtt_api/doc/mqtt_api_overview.md) and [MQTT API Interface](components/mqtt_api/doc/mqtt_api_interface.md) in sync

5. **Code Comments**
   - Add or update code comments to reflect changes
   - Document public APIs and interfaces
   - Explain complex algorithms or non-obvious behavior

## Additional Guidelines

### Component Architecture

- **Loose Coupling**: Components should be loosely coupled through the `component_connector` component
- **Separation of Concerns**: Keep components focused on their specific responsibilities
- **Interface Design**: Design clean, well-defined interfaces between components

### Command Class Development

For implementing new Z-Wave command classes, refer to the comprehensive [Command Class Implementation Guide](docs/command_class_implementation_guide.md). This guide covers:

- Command class architecture
- MQTT interface implementation
- Attribute store integration
- Event handling
- Testing requirements

If you are using Cursor, the [generate-new-cc skill](.cursor/skills/generate-new-cc/SKILL.md) provides step-by-step AI-assisted guidance for generating command class boilerplate and wiring up a new implementation.

### Module Implementation

- **Language**: New modules must be implemented in C++
- **C Interoperability**: If a new C++ component needs to be used in C modules, create a C wrapper
- **Existing Patterns**: Follow existing patterns and conventions in the codebase

### General Best Practices

- **Keep Changes Focused**: Each PR should address a single issue or feature
- **Incremental Development**: Break large features into smaller, reviewable PRs
- **Backward Compatibility**: Consider backward compatibility when making changes
- **Performance**: Be mindful of performance implications, especially in critical paths
- **Security**: Follow security best practices, especially when handling network data or user input

## Getting Help

If you have questions or need help:

1. Check existing documentation (README.md, implementation guides, etc.)
2. Search existing GitHub issues for similar questions
3. Open a new GitHub issue using the [Feature Request template](.github/ISSUE_TEMPLATE/feature.yaml) for proposals, or a [Bug Report](.github/ISSUE_TEMPLATE/bug.yaml) for unexpected behavior
4. Reach out to maintainers through appropriate internal channels

## Thank You

Thank you for taking the time to contribute to ZPC! Your contributions help make this project better for everyone. We appreciate your effort and look forward to your contributions.
