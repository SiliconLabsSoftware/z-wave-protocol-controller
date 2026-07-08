#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

case "${1:-check}" in
check) ;;
-h | --help | help)
  echo "Usage: ci/scripts/clang-tidy.sh check"
  exit 0
  ;;
*)
  echo "clang-tidy: unknown command: $1" >&2
  exit 1
  ;;
esac

case "$(uname -s)" in
Darwin)
  preset="${CMAKE_PRESET:-macos}"
  jobs="$(sysctl -n hw.ncpu)"
  tidy="${CLANG_TIDY:-}"
  runner="${RUN_CLANG_TIDY:-}"
  if [[ -z "$tidy" ]]; then
    llvm="$(brew --prefix llvm@19 2>/dev/null || true)"
    if [[ -n "$llvm" && -x "$llvm/bin/clang-tidy" ]]; then
      tidy="$llvm/bin/clang-tidy"
      runner="${runner:-$llvm/bin/run-clang-tidy}"
    fi
  fi
  ;;
*)
  preset="${CMAKE_PRESET:-debian}"
  jobs="$(nproc)"
  tidy="${CLANG_TIDY:-clang-tidy-19}"
  runner="${RUN_CLANG_TIDY:-run-clang-tidy-19}"
  ;;
esac

if ! command -v "$tidy" >/dev/null 2>&1 || ! command -v "$runner" >/dev/null 2>&1; then
  echo "clang-tidy: install LLVM 19 tools from ci/dependencies/brew-packages.txt or ci/dependencies/apt-packages-clang-tidy.txt (with apt-packages-base.txt)" >&2
  exit 1
fi

cmake --preset "$preset"

extra=()
if [[ "$(uname -s)" == Darwin ]]; then
  sdk="$(xcrun --show-sdk-path)"
  extra=(-extra-arg=-isysroot"$sdk" -extra-arg=-isystem"$sdk/usr/include/c++/v1")
fi

exec "$runner" -p "build/$preset" -j "$jobs" -config-file=.clang-tidy \
  -clang-tidy-binary="$tidy" "${extra[@]}" -quiet applications components
