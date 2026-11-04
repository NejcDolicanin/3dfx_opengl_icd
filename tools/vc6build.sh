#!/bin/bash
# Build with the real Visual C++ 6.0 toolchain (Devtools\msvc6_0\VC98) under the system Wine.
#   tools/vc6build.sh              -> nmake -f makefile.vc6 objs   (compile only)
#   tools/vc6build.sh all          -> nmake -f makefile.vc6        (compile and link build/out/3dfxogl.dll)
#   EXTRA=/DICD_TRACE tools/vc6build.sh all                        (extra compiler flags: here the trace build)
#   tools/vc6build.sh clean
#   tools/vc6build.sh cl <file.c>  -> compile one source file to build\out\ (VC6 syntax check)
#   tools/vc6build.sh dumpbin ...  -> run DUMPBIN.EXE
set -e
HERE="$(cd "$(dirname "$0")/.." && pwd)"
export WINEPREFIX=/home/nejc/work_utility/wineprefix-vc6
export WINEDEBUG=${WINEDEBUG:--all}
export WINEDLLOVERRIDES="mscoree,mshtml="
DEVTOOLS=/mnt/4c4334e3-42c1-428b-934d-dae5f30fc6ce/Retro/DriverDevelopment/_work/Devtools
VC98="$DEVTOOLS/msvc6_0/VC98"
SDK="$DEVTOOLS/win32sdk"
[ -d "$WINEPREFIX" ] || { echo "creating wine prefix..."; wineboot -i >/dev/null 2>&1; }
w() { winepath -w "$1"; }
# compiler: VC6 RTM (build 8168, as the original) unless VC6BIN=sp
if [ "${VC6BIN:-rtm}" = "sp" ]; then BIN="$VC98/Bin"; else BIN="$HERE/toolchain/vc6rtm/Bin"; fi
export WINEPATH="$(w "$BIN");$(w "$SDK/bin")"
export INCLUDE="$(w "$VC98/Include");$(w "$SDK/include")"
export LIB="$(w "$VC98/Lib");$(w "$SDK/lib")"
export VC42INC="$(w "$DEVTOOLS/Msvc4_2/MSDEV/include")"      # Texus: VC 4.2's headers (makefile.vc6 TXFLAGS)
cd "$HERE/build"
mkdir -p out
case "${1:-objs}" in
  cl)      shift
           wine "$BIN/CL.EXE" /nologo /c /MD /W3 /Gr /O2 /Zp4 /Gy \
             /DWIN32 /D_WINDOWS /D__GL_WIN32 /DFX_GLIDE_H5 /D__WIN32__ /DGLIDE_LIB \
             '/I..\src\include' '/I..\src\sdk\glide3x\include' \
             '/Foout\' "$@" ;;
  dumpbin) shift; wine "$BIN/DUMPBIN.EXE" "$@" ;;
  ccraw)   shift; wine "$BIN/CL.EXE" /nologo "$@" ;;         # raw CL.EXE with the caller's flags (cwd build/)
  *)       wine "$BIN/NMAKE.EXE" /nologo -f makefile.vc6 "$@" ;;
esac
