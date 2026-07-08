#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
chmod +x "$repo_root/ci/scripts/clang-format.sh"
chmod +x "$repo_root/.githooks/pre-commit"
git -C "$repo_root" config core.hooksPath .githooks
git -C "$repo_root" config clangFormat.extensions c,cpp,h,hpp
git -C "$repo_root" config clangFormat.style file
echo "Installed pre-commit hook (core.hooksPath=.githooks)"
