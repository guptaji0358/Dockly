/* The whole contract between Dockly's C++ code and the Rust DLL (dockly_native.dll).
 *
 * Rules
 *  - C ABI only. Symbols start with dn_. Fixed-width integer types.
 *  - The caller owns every buffer. No allocation crosses the boundary.
 *  - Every call that can fail returns a dn status; results go through out-pointers.
 *  - Nothing unwinds across the boundary: a Rust panic becomes DN_ERR_PANIC.
 *  - Structs begin with struct_size, which the caller sets to sizeof(the struct) before the call.
 *  - Changing a signature, a layout or the meaning of a field bumps DN_ABI_VERSION.
 *
 * Keep this file in step with src/lib.rs and src/sysinfo.rs.
 */
#ifndef DOCKLY_NATIVE_H
#define DOCKLY_NATIVE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DN_ABI_VERSION 1u

#define DN_OK 0
#define DN_ERR_ARG (-1)   /* null or misaligned pointer */
#define DN_ERR_SIZE (-2)  /* struct_size is smaller than this library's struct */
#define DN_ERR_OS (-3)    /* a Windows call failed */
#define DN_ERR_PANIC (-4) /* internal error in the library */

#ifdef __cplusplus
}
#endif

#endif /* DOCKLY_NATIVE_H */
