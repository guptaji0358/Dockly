/* The whole contract between Dockly's C++ code and the Rust DLL (dockly_native.dll).
 *
 * Rules
 *  - C ABI only. Symbols start with dn_. Fixed-width integer types.
 *  - The caller owns every buffer. No allocation crosses the boundary.
 *  - Every call that can fail returns a dn status; results go through out-pointers.
 *  - Nothing unwinds across the boundary: a Rust panic becomes DN_ERR_PANIC.
 *  - Single structs begin with struct_size, which the caller sets to sizeof(the struct) before
 *    the call. Arrays pass the element size, which must match exactly.
 *  - Long-lived state is a numeric handle with dn_*_open / dn_*_close. A closed or unknown handle
 *    is refused with DN_ERR_ARG.
 *  - Text is NUL-terminated UTF-8 in fixed-size fields, cut at a character boundary if too long.
 *  - Every function is safe to call from any thread.
 *  - Changing a signature, a layout or the meaning of a field bumps DN_ABI_VERSION.
 *
 * Keep this file in step with src/lib.rs and the #[repr(C)] structs beside it.
 */
#ifndef DOCKLY_NATIVE_H
#define DOCKLY_NATIVE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DN_ABI_VERSION 2u

#define DN_OK 0
#define DN_ERR_ARG (-1)   /* null or misaligned pointer, unknown kind or handle */
#define DN_ERR_SIZE (-2)  /* struct_size too small, or element size wrong */
#define DN_ERR_OS (-3)    /* a Windows call failed */
#define DN_ERR_PANIC (-4) /* internal error in the library */
#define DN_ERR_MORE (-5)  /* the array is too small; *count says how many elements are needed */
#define DN_ERR_LIMIT (-6) /* too many open handles */

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

/* This process's memory. The address space totals come from a region walk that is not atomic.
 * native_* count only the DLL's own heap, not Qt or C++ allocations. */
typedef struct dn_memory_info {
    uint32_t struct_size;
    uint32_t handle_count;
    uint64_t working_set_bytes;
    uint64_t peak_working_set_bytes;
    uint64_t private_bytes;     /* committed private memory (commit charge) */
    uint64_t peak_commit_bytes;
    uint64_t page_faults;
    uint64_t committed_private_bytes; /* heaps, stacks, other private regions */
    uint64_t committed_mapped_bytes;  /* mapped files and shared memory */
    uint64_t committed_image_bytes;   /* loaded EXE and DLL images */
    uint64_t reserved_bytes;
    uint64_t free_bytes;
    uint64_t largest_free_block_bytes;
    uint64_t free_block_count;
    uint64_t native_live_bytes;
    uint64_t native_peak_bytes;
    uint64_t native_allocations;
    uint64_t native_frees;
} dn_memory_info;

/* dn_device.kind */
#define DN_DEVICE_STORAGE 1u   /* drive letters */
#define DN_DEVICE_USB 2u
#define DN_DEVICE_HID 3u
#define DN_DEVICE_SERIAL 4u    /* COM ports */
#define DN_DEVICE_BLUETOOTH 5u
#define DN_DEVICE_NETWORK 6u   /* adapters, virtual ones included */
#define DN_DEVICE_DISPLAY 7u   /* GPUs and virtual display adapters */
#define DN_DEVICE_SENSOR 8u

/* dn_device.flags */
#define DN_FLAG_STARTED 1u   /* driver running; for storage: volume readable */
#define DN_FLAG_PROBLEM 2u   /* Windows reports a problem code in .problem */
#define DN_FLAG_REMOVABLE 4u /* removable or optical drive */
#define DN_FLAG_REMOTE 8u    /* network drive; not queried further */
#define DN_FLAG_HAS_SIZE 16u /* total_bytes and free_bytes are valid */

#define DN_NAME_LEN 256
#define DN_ID_LEN 512
#define DN_DETAIL_LEN 128

typedef struct dn_device {
    uint32_t kind;
    uint32_t flags;
    uint32_t problem;
    uint16_t vendor_id;  /* from VID_xxxx in the id, else 0 */
    uint16_t product_id; /* from PID_xxxx in the id, else 0 */
    uint64_t total_bytes;
    uint64_t free_bytes;
    char name[DN_NAME_LEN];
    char id[DN_ID_LEN];         /* device instance id, or "C:\" for storage; may hold serial numbers */
    char detail[DN_DETAIL_LEN]; /* manufacturer, or the file system for storage */
} dn_device;

typedef struct dn_watch_counts {
    uint32_t struct_size;
    uint32_t reserved;
    uint64_t arrivals; /* device instances started since dn_watch_open */
    uint64_t removals; /* device instances removed since dn_watch_open */
} dn_watch_counts;

/* The ABI version the DLL was built with. Compare with DN_ABI_VERSION before any other call. */
uint32_t dn_abi_version(void);

/* Fills *out with a read-only snapshot.
 * out must point to a writable object of at least out->struct_size bytes.
 * On any status other than DN_OK, *out is left unchanged. The same holds for the two calls below
 * that take a single struct. */
int32_t dn_system_info_read(dn_system_info *out);
int32_t dn_memory_info_read(dn_memory_info *out);

/* Lists the present devices of one kind. items may be null when capacity is 0. item_size must be
 * sizeof(dn_device). *count receives the number found on DN_OK and DN_ERR_MORE; items are written
 * only on DN_OK. The list can change between calls, so retry on DN_ERR_MORE.
 * May take milliseconds; do not call it every frame. */
int32_t dn_device_list(uint32_t kind, dn_device *items, uint32_t capacity, uint32_t item_size,
                       uint32_t *count);

/* Watches for devices being connected and removed. Poll with dn_watch_read; nothing calls back.
 * Close every handle before unloading the DLL. *handle is written only on DN_OK. */
int32_t dn_watch_open(uint64_t *handle);
int32_t dn_watch_read(uint64_t handle, dn_watch_counts *out);
int32_t dn_watch_close(uint64_t handle);

#ifdef __cplusplus
}
#endif

#endif /* DOCKLY_NATIVE_H */
