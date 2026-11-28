//! Exports only. The contract is `include/dockly_native.h`.
#![deny(unsafe_op_in_unsafe_fn)]

mod ffi;
mod sysinfo;

pub use sysinfo::SystemInfo;

/// Must equal `DN_ABI_VERSION` in the header.
const ABI_VERSION: u32 = 1;

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
