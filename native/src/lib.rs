//! Exports only. The contract is `include/dockly_native.h`.
#![deny(unsafe_op_in_unsafe_fn)]

mod devices;
mod ffi;
mod memory;
mod sysinfo;
mod watch;

pub use devices::Device;
pub use memory::MemoryInfo;
pub use sysinfo::SystemInfo;
pub use watch::WatchCounts;

/// Must equal `DN_ABI_VERSION` in the header.
const ABI_VERSION: u32 = 2;

#[no_mangle]
pub extern "C" fn dn_abi_version() -> u32 {
    ABI_VERSION
}

/// # Safety
/// `out` is null, or points to a writable object of at least `out->struct_size` bytes.
#[no_mangle]
pub unsafe extern "C" fn dn_system_info_read(out: *mut SystemInfo) -> i32 {
    // SAFETY: the caller's promise above is exactly what `write_out` requires.
    ffi::guard(|| unsafe { ffi::write_out(out, sysinfo::read) })
}

/// # Safety
/// `out` is null, or points to a writable object of at least `out->struct_size` bytes.
#[no_mangle]
pub unsafe extern "C" fn dn_memory_info_read(out: *mut MemoryInfo) -> i32 {
    // SAFETY: as above.
    ffi::guard(|| unsafe { ffi::write_out(out, memory::read) })
}

/// # Safety
/// `items` is null with `capacity` 0, or points to `capacity` writable elements of `item_size`
/// bytes. `count` is null or points to a writable u32.
#[no_mangle]
pub unsafe extern "C" fn dn_device_list(
    kind: u32,
    items: *mut Device,
    capacity: u32,
    item_size: u32,
    count: *mut u32,
) -> i32 {
    // SAFETY: the caller's promise above is exactly what `write_list` requires.
    ffi::guard(|| unsafe { ffi::write_list(items, capacity, item_size, count, || devices::list(kind)) })
}

/// # Safety
/// `handle` is null or points to a writable u64.
#[no_mangle]
pub unsafe extern "C" fn dn_watch_open(handle: *mut u64) -> i32 {
    ffi::guard(|| {
        if handle.is_null() || !handle.is_aligned() {
            return ffi::ERR_ARG;
        }
        match watch::open() {
            Ok(opened) => {
                // SAFETY: non-null and aligned (checked), writable by the caller's promise.
                unsafe { handle.write(opened) };
                ffi::OK
            }
            Err(status) => status,
        }
    })
}

/// # Safety
/// `out` is null, or points to a writable object of at least `out->struct_size` bytes.
#[no_mangle]
pub unsafe extern "C" fn dn_watch_read(handle: u64, out: *mut WatchCounts) -> i32 {
    // SAFETY: as for `write_out`.
    ffi::guard(|| unsafe { ffi::write_out(out, || watch::counts(handle)) })
}

#[no_mangle]
pub extern "C" fn dn_watch_close(handle: u64) -> i32 {
    ffi::guard(|| watch::close(handle).map_or_else(|status| status, |()| ffi::OK))
}
