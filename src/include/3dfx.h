/*
** 3dfx.h -- wrapper picked up before the Glide SDK's own 3dfx.h (src/include is first on the
** include path; glide.h says `#include <3dfx.h>`).  The SDK copy is src/sdk/glide3x/include/3dfx.h.
**
** The SDK declares every Glide entry point __declspec(dllimport) for MSC, which makes VC6 emit
** `call [__imp__grX@N]`.  3dfxOGL.dll calls Glide through the linker's `jmp [IAT]` thunks
** (`call 0x6906836e` -> `jmp [0x690012..]`), i.e. it was built with plain prototypes: FX_ENTRY is
** redefined accordingly.  FX_CALL stays __stdcall.
*/
#ifndef __ICD_3DFX_WRAPPER_H__
#define __ICD_3DFX_WRAPPER_H__
#include "../sdk/glide3x/include/3dfx.h"
#undef  FX_ENTRY
#define FX_ENTRY extern
#endif
