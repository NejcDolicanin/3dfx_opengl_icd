The Glide 3.x SDK files the ICD is built against, copied so the project builds on its own.

Source: the sourceforge Glide 3 tree (glide3x, Voodoo4/5 "h5" build), copied 2026-11-02:
  include/glide.h glidesys.h glideutl.h g3ext.h   <- glide3x/h5/glide3/src
  include/sst1vid.h                              <- glide3x/h5/incsrc
  include/3dfx.h fx64.h                          <- glide3x/swlibs/fxmisc
  lib/glide3x.lib                                <- 3dx Source H5/LIB/GLIDE3X.LIB (import library of glide3x.dll;
                                                    the one the original was linked with: all 59 hints equal,
                                                    2026-11-03; the sourceforge one is archived)

Only the files the build includes are here.  src/include/3dfx.h wraps include/3dfx.h (FX_ENTRY without
dllimport, so Glide is called through the linker's jmp thunks as in the original).  Do not edit these files.
