The Glide / 3dfx headers Texus is compiled against: 3dfx's own tree of mid-2000 ("3dx Source", the drop that
src/texus2 comes from), as 3dfx's SWLIBS/TEXUS2/LIB/MAKEFILE did (-I$(BUILD_ROOT_SWLIBS)\include
-I$(BUILD_ROOT)\$(FX_GLIDE_HW)\include).

  include/3dfx.h fx64.h                                <- 3dx Source/SWLIBS/INCLUDE
  include/glide.h glidesys.h glideutl.h sst1vid.h      <- 3dx Source/H5/INCLUDE

Only Texus uses these (makefile.vc6 TXFLAGS); the rest of the ICD uses src/sdk/glide3x.  They differ from the
sourceforge copies in src/sdk/glide3x, and Visual C++ 4.2's register allocation reacts to the declarations a
file sees: with these headers 201 of 212 Texus functions compile identical to the original, with the sourceforge
ones 151.  Do not edit these files.
