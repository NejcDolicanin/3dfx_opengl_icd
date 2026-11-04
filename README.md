# 3dfx_opengl_icd

A decompilation of the 3dfx OpenGL ICD `3dfxOGL.dll` (1.0.0.0761, Voodoo 4/5) into maintainable C that rebuilds with Visual C++ 6.0.

## What you need/was used to build

The same tools 3dfx used, so the DLL comes out like the original. The Microsoft parts are not in git (not ours to redistribute); copy them to the places below.

**Host**
- Linux with Wine. `tools/vc6build.sh` makes its own Wine prefix on first run.
- Or Windows without Wine: write a .bat version of `tools/vc6build.sh` (it only sets PATH / INCLUDE / LIB and runs
  NMAKE).

**Compilers** (into `toolchain/`)
- `toolchain/vc6rtm/Bin`: Visual C++ 6.0 **RTM**, no service pack: CL.EXE 12.00.8168 and LINK.EXE / NMAKE.EXE
  6.00.8168.  CL.EXE, C1.DLL, C1XX.DLL, C2.DLL and LINK.EXE from `VC98\BIN` on Visual Studio 6.0 CD1 (not an
  installed copy: a service pack replaces them).  The rest of the folder (NMAKE.EXE, MSPDB60.DLL, ...) from any
  VC6 install.
- `toolchain/vc42/Bin`: Visual C++ 4.2, cl.exe 10.20.6166 (cl.exe, c1.exe, c2.exe, mspdb41.dll and the .err /
  .msg files).  Only Texus (`src/texus2`) is built with it, as 3dfx did.

**Headers and resource compiler** (in `../Devtools`, read in place)
- `msvc6_0/VC98`: an installed Visual C++ 6.0; its `Include` has the Windows and DirectDraw headers (`DDRAW.H`).
- `win32sdk`: Microsoft Win32 SDK (August 1996): more headers, and `bin/RC.EXE` 4.00 for `build/3dfxogl.rc`.
- `Msvc4_2/MSDEV/include`: Visual C++ 4.2's headers, for Texus.

**Import libraries** (into `toolchain/lib`; the same ones the original was linked with)
- `KERNEL32.LIB`, `USER32.LIB`, `GDI32.LIB`, `ADVAPI32.LIB`: Win32 SDK, `lib`.
- `MSVCRT.LIB`: Windows 98 DDK, `lib/i386/free`.
- `DDRAW.LIB`: Visual C++ 6.0, `VC98/Lib`.
- `dx60sdk/DXGUID.LIB`: DirectX 6.0 SDK (`LIB\DXGUID.LIB`; on archive.org as "dx-6-sdk", Dx6SDK.iso).  It must be
  6.0: the 6.1 and 7 versions hold different GUID sets.

**Already in the project**
- The Glide 3 headers and `glide3x.lib` (`src/sdk/glide3x`) and the older Glide headers Texus needs
  (`src/sdk/glide3x_texus`).

**Paths to edit**: `tools/vc6build.sh` has two absolute paths near the top, `WINEPREFIX` (where Wine keeps the
prefix) and `DEVTOOLS` (the `Devtools` folder above).

## Build

```sh
tools/vc6build.sh clean
tools/vc6build.sh all          # -> build/out/3dfxogl.dll
```
