#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
CC="${CC:-cc}"
STD="${STD:-c99}"
CFLAGS="${CFLAGS:--std=$STD -O2 -Wall -Wextra -Wpedantic -Werror}"

mkdir -p "$BUILD_DIR"

if [ "$#" -gt 0 ]; then
    test_files=("$@")
else
    test_files=()
    for f in "$ROOT_DIR"/tests/test_*.c; do
        [ -e "$f" ] && test_files+=("$f")
    done
fi

if [ "${#test_files[@]}" -eq 0 ]; then
    echo "No test files found in $ROOT_DIR/tests/test_*.c"
    exit 0
fi

passed=0
failed=0

for src in "${test_files[@]}"; do
    name="$(basename "$src" .c)"
    bin="$BUILD_DIR/$name"
    echo "==> Building & running $name"
    # shellcheck disable=SC2086
    if ! "$CC" $CFLAGS -I"$ROOT_DIR" -I"$ROOT_DIR/tests" "$src" -o "$bin"; then
        echo "COMPILE FAILED: $src"
        failed=$((failed + 1))
        continue
    fi
    if "$bin"; then
        passed=$((passed + 1))
    else
        echo "TEST FAILED: $name"
        failed=$((failed + 1))
    fi
done

echo ""
echo "Summary: $passed suite(s) passed, $failed suite(s) failed."
[ "$failed" -eq 0 ]
