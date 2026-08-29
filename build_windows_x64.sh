#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
OUT="$ROOT/out-windows-x64"
CLANG=${CLANG:-/usr/local/swift/usr/bin/clang++}
LLD=${LLD:-/usr/local/swift/usr/bin/lld-link}

mkdir -p "$OUT"

for lib in kernel32 user32 gdi32 gdiplus winmm; do
    "$LLD" /lib /def:"$ROOT/windows-build/$lib.def" /machine:x64 /out:"$OUT/$lib.lib"
done

"$CLANG" --target=x86_64-pc-windows-msvc \
    -I"$ROOT/src/windows" \
    -c "$ROOT/src/windows/V_mini_me_win.cpp" \
    -o "$OUT/V_mini_me_win.obj" \
    -O2 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector \
    -fno-builtin -nostdlib -nostdinc++

"$LLD" \
    /out:"$OUT/V_mini_me.exe" \
    /entry:mainCRTStartup \
    /subsystem:console,6.01 \
    /machine:x64 \
    /opt:ref /opt:icf \
    "$OUT/V_mini_me_win.obj" \
    "$OUT/kernel32.lib" "$OUT/user32.lib" "$OUT/gdi32.lib" \
    "$OUT/gdiplus.lib" "$OUT/winmm.lib"

printf 'Built %s\n' "$OUT/V_mini_me.exe"
