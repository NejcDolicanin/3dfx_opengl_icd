#!/bin/bash
# Build tests/gltest/gltest.c with the Visual C++ 6.0 toolchain (see tools/vc6build.sh) -> build/out/gltest.exe
set -e
HERE="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$HERE/build/out"
"$HERE/tools/vc6build.sh" ccraw /W3 /O2 /ML '/Feout\gltest.exe' '/Foout\gltest.obj' '..\tests\gltest\gltest.c' \
    /link /SUBSYSTEM:WINDOWS opengl32.lib gdi32.lib user32.lib
ls -l "$HERE/build/out/gltest.exe"
