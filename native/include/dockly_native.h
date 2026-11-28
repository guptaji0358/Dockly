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

typedef struct dn_system_info {
    uint32_t struct_size;
    uint32_t logical_processors; /* all processor groups */
    uint32_t physical_cores;
    uint32_t processor_packages;
    uint64_t total_physical_bytes;
    uint64_t available_physical_bytes;
    uint32_t os_major;
    uint32_t os_minor;
    uint32_t os_build;
    uint32_t memory_load_percent; /* 0..100 */
} dn_system_info;

/* The ABI version the DLL was built with. Compare with DN_ABI_VERSION before any other call. */
uint32_t dn_abi_version(void);

/* Fills *out with a read-only snapshot. Safe to call from any thread.
 * out must point to a writable object of at least out->struct_size bytes.
 * On any status other than DN_OK, *out is left unchanged. */
int32_t dn_system_info_read(dn_system_info *out);

#ifdef __cplusplus
}
#endif

#endif /* DOCKLY_NATIVE_H */
