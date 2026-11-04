Visual C++ 6.0 RTM compiler and linker (build 8168), the toolchain 3dfxOGL.dll 1.0.0.0761 was built
with (its PE Rich header: c2 / link build 8168).  CL.EXE, C1.DLL, C1XX.DLL, C2.DLL, LINK.EXE come from
VC98\BIN on CD1 of Devtools/en_vs6_ent.zip (Visual Studio 6.0 Enterprise); the other files are copies
of Devtools/msvc6_0/VC98/Bin (service pack, unchanged tools).  tools/vc6build.sh uses this Bin by
default; VC6BIN=sp uses the installed service-pack compiler instead.
