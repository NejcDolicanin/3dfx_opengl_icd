The import libraries 3dfxOGL.dll 1.0.0.0761 was linked against, copied so the project links on its own.
Found by their hints: every hint and name in the original's import tables equals the library's.

  KERNEL32.LIB USER32.LIB GDI32.LIB ADVAPI32.LIB  <- Devtools/win32sdk/lib        (32/32, 19/19, 22/22, 3/3)
  MSVCRT.LIB                                      <- Devtools/win98ddk/lib/i386/free (39/39; its static chkstk /
                                                     llshl / ullshr are byte-identical to VC98's)
  DDRAW.LIB                                       <- Devtools/msvc6_0/VC98/Lib    (1/1)

The Glide import library is src/sdk/glide3x/lib/glide3x.lib (3dx Source H5/LIB/GLIDE3X.LIB, 59/59).
A hint only speeds up the loader's name lookup: the names are the same with any of these libraries.

  dx60sdk/DXGUID.LIB  <- DirectX 6.0 SDK, LIB\DXGUID.LIB (archive.org item dx-6-sdk, Dx6SDK.iso; md5 b2ece0612ebc56c3dba83c169c5a033d).
                 Its one object holds 379 GUIDs in one .rdata section, byte-identical to the original's block at
                 69004238..690059e8: the original's code uses IID_IDirectDraw2 from it (sstsurf.c), which pulls in
                 the whole section.  (DX 6.1's has 391 GUIDs in another order; DX 7's 420 as separate COMDATs.)
