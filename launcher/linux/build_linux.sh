#!/usr/bin/env sh
set -eu

if [ "$#" -lt 1 ]; then
    echo "usage: $0 /path/to/raylib-6.0_linux_amd64 [output]" >&2
    exit 2
fi

RAYLIB_DIR=$1
OUTPUT=${2:-V_mini_me_launcher}
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

c++ -O2 -std=c++17 -Wall -Wextra -Wpedantic \
    -I"$RAYLIB_DIR/include" \
    "$SCRIPT_DIR/V_mini_me_launcher.cpp" \
    "$RAYLIB_DIR/lib/libraylib.a" \
    -lGL -lm -lpthread -ldl -lrt -lX11 \
    -o "$OUTPUT"

strip "$OUTPUT" 2>/dev/null || true
printf 'built %s\n' "$OUTPUT"
