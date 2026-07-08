#!/usr/bin/env bash
set -euo pipefail

readonly CPP_EXTENSIONS=c,cpp,h,hpp

usage() {
  cat <<'EOF'
Usage: ci/scripts/clang-format.sh <command> [options]

Commands:
  format          Format all hand-written C/C++ sources
  check           Verify formatting (--dry-run --Werror)

Options:
  --staged        With format: format staged C/C++ files and re-stage them (pre-commit)
                  With check: verify staged C/C++ files only (test before commit)

Skips paths listed in .clang-format-ignore. Uses LLVM 19 clang-format (macOS: brew llvm@19;
Linux: clang-format-19). Override with CLANG_FORMAT or GIT_CLANG_FORMAT.
Staged commands use git-clang-format; full-tree commands use clang-format directly.
EOF
}

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$repo_root"

clang_format_binary() {
  if [[ -n "${CLANG_FORMAT:-}" ]]; then
    if ! command -v "$CLANG_FORMAT" >/dev/null 2>&1; then
      echo "clang-format: CLANG_FORMAT=$CLANG_FORMAT not found" >&2
      exit 1
    fi
    echo "$CLANG_FORMAT"
    return
  fi

  if command -v brew >/dev/null 2>&1; then
    llvm_prefix="$(brew --prefix llvm@19 2>/dev/null || true)"
    if [[ -n "$llvm_prefix" && -x "$llvm_prefix/bin/clang-format" ]]; then
      echo "$llvm_prefix/bin/clang-format"
      return
    fi
  fi

  if command -v clang-format-19 >/dev/null 2>&1; then
    echo clang-format-19
    return
  fi

  echo "clang-format: install LLVM 19 clang-format (macOS: brew install llvm@19; Linux: clang-format-19)" >&2
  exit 1
}

git_clang_format_binary() {
  if [[ -n "${GIT_CLANG_FORMAT:-}" ]]; then
    if ! command -v "$GIT_CLANG_FORMAT" >/dev/null 2>&1; then
      echo "clang-format: GIT_CLANG_FORMAT=$GIT_CLANG_FORMAT not found" >&2
      exit 1
    fi
    echo "$GIT_CLANG_FORMAT"
    return
  fi

  if command -v brew >/dev/null 2>&1; then
    llvm_prefix="$(brew --prefix llvm@19 2>/dev/null || true)"
    if [[ -n "$llvm_prefix" && -x "$llvm_prefix/bin/git-clang-format" ]]; then
      echo "$llvm_prefix/bin/git-clang-format"
      return
    fi
  fi

  if command -v git-clang-format-19 >/dev/null 2>&1; then
    echo git-clang-format-19
    return
  fi

  if command -v git-clang-format >/dev/null 2>&1; then
    echo git-clang-format
    return
  fi

  echo "clang-format: install git-clang-format (macOS: brew install llvm@19; Linux: clang-format-19)" >&2
  exit 1
}

list_cpp_files() {
  # Performance: prune directories listed in .clang-format-ignore (clang-format skips them too).
  find . \( -path ./build -o -path ./.venv -o -path ./.venv-docs -o -path '*/generated' \) -prune -o \
    \( -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) \
    -print0
}

xargs_flags=()
if xargs --help 2>&1 | grep -q -- '-r'; then
  xargs_flags=(-r)
fi

run_on_cpp_files() {
  local clang_format="$1"
  shift
  list_cpp_files | xargs -0 "${xargs_flags[@]}" "$clang_format" "$@"
}

run_git_clang_format_staged() {
  local mode="$1"
  local clang_format git_clang_format exit_code
  local -a restage=()
  local file

  clang_format="$(clang_format_binary)"
  git_clang_format="$(git_clang_format_binary)"

  while IFS= read -r file; do
    [[ -n "$file" ]] && restage+=("$file")
  done < <(git diff --cached --name-only --diff-filter=ACMR)

  if [[ ${#restage[@]} -eq 0 ]]; then
    return 0
  fi

  if [[ "$mode" == check ]]; then
    set +e
    "$git_clang_format" --staged --diff \
      --extensions "$CPP_EXTENSIONS" \
      --binary "$clang_format" --style=file
    exit_code=$?
    set -e
    case "$exit_code" in
      0)
        return 0
        ;;
      1)
        echo "clang-format: staged changes need formatting" >&2
        echo "clang-format: run './ci/scripts/clang-format.sh format --staged' to fix" >&2
        return 1
        ;;
      *)
        echo "clang-format: could not check staged formatting" >&2
        return 1
        ;;
    esac
  fi

  set +e
  "$git_clang_format" --staged \
    --extensions "$CPP_EXTENSIONS" \
    --binary "$clang_format" --style=file
  exit_code=$?
  set -e
  case "$exit_code" in
    0|1)
      git add -- "${restage[@]}"
      return 0
      ;;
    *)
      echo "clang-format: could not format staged changes" >&2
      return 1
      ;;
  esac
}

cmd_format() {
  local staged=false
  if [[ "${1:-}" == --staged ]]; then
    staged=true
  elif [[ -n "${1:-}" ]]; then
    echo "clang-format: unknown option for format: $1" >&2
    usage >&2
    exit 1
  fi

  if [[ "$staged" == true ]]; then
    run_git_clang_format_staged format
    return
  fi

  local clang_format
  clang_format="$(clang_format_binary)"
  run_on_cpp_files "$clang_format" -i --style=file
}

cmd_check() {
  local staged=false
  if [[ "${1:-}" == --staged ]]; then
    staged=true
  elif [[ -n "${1:-}" ]]; then
    echo "clang-format: unknown option for check: $1" >&2
    usage >&2
    exit 1
  fi

  if [[ "$staged" == true ]]; then
    run_git_clang_format_staged check
    return
  fi

  local clang_format
  clang_format="$(clang_format_binary)"
  run_on_cpp_files "$clang_format" --style=file --dry-run --Werror
}

command="${1:-}"
shift || true

case "$command" in
  format) cmd_format "$@" ;;
  check) cmd_check "$@" ;;
  -h|--help|help|"") usage ;;
  *)
    echo "clang-format: unknown command: $command" >&2
    usage >&2
    exit 1
    ;;
esac
