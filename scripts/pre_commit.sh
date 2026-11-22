#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR="${BUILD_DIR:-build}"
MODE="${1:-check}"

cd "$ROOT_DIR"

if [[ "$MODE" == "format" ]]; then
  if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format is required for formatting" >&2
    exit 1
  fi
elif [[ "$MODE" == "check" || "$MODE" == "lint" || "$MODE" == "all" ]]; then
  if [[ "$MODE" != "lint" ]] && ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format is required for format-check" >&2
    exit 1
  fi
  if [[ "$MODE" != "check" ]] && ! command -v clang-tidy >/dev/null 2>&1; then
    echo "clang-tidy is required for linting" >&2
    exit 1
  fi
else
  echo "usage: scripts/pre_commit.sh [format|check|lint|all]" >&2
  exit 2
fi

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

case "$MODE" in
  format)
    cmake --build "$BUILD_DIR" --target format
    ;;
  check)
    cmake --build "$BUILD_DIR" --target format-check
    ;;
  lint)
    cmake --build "$BUILD_DIR" --parallel --target test_spsc_ring test_signal test_crc_counter
    cmake --build "$BUILD_DIR" --target lint
    ;;
  all)
    cmake --build "$BUILD_DIR" --target format-check
    cmake --build "$BUILD_DIR" --parallel --target test_spsc_ring test_signal test_crc_counter
    cmake --build "$BUILD_DIR" --target lint
    ctest --test-dir "$BUILD_DIR" --output-on-failure
    ;;
esac
