//! Boundary checks: status codes, panic containment and caller-owned output structs.

use std::panic::{catch_unwind, AssertUnwindSafe};

pub const OK: i32 = 0;
pub const ERR_ARG: i32 = -1;
pub const ERR_SIZE: i32 = -2;
pub const ERR_OS: i32 = -3;
pub const ERR_PANIC: i32 = -4;

/// Runs an export body so that a panic never unwinds into C++.
pub fn guard(body: impl FnOnce() -> i32) -> i32 {
    catch_unwind(AssertUnwindSafe(body)).unwrap_or(ERR_PANIC)
}

/// An output struct.
///
/// # Safety
/// Implementors are `#[repr(C)]`, contain only integers, and their first field is a `u32`
/// holding the struct size.
pub unsafe trait OutStruct: Copy {
    fn set_struct_size(&mut self, size: u32);
}

/// Validates `out`, produces a value and copies it to the caller. `out` is written only on success.
///
/// # Safety
/// `out` is null, or points to a writable object of at least as many bytes as its first `u32` says.
pub unsafe fn write_out<T: OutStruct>(out: *mut T, produce: impl FnOnce() -> Result<T, i32>) -> i32 {
    if out.is_null() || !out.is_aligned() {
        return ERR_ARG;
    }
    // SAFETY: non-null and aligned; `T` starts with a u32, which the caller has initialised.
    let given = unsafe { out.cast::<u32>().read() };
    if (given as usize) < size_of::<T>() {
        return ERR_SIZE;
    }
    match produce() {
        Ok(mut value) => {
            // a newer caller may pass a larger struct: keep its size, fill the part known here
            value.set_struct_size(given);
            // SAFETY: the object holds at least `size_of::<T>()` bytes (checked above).
            unsafe { out.write(value) };
            OK
        }
        Err(status) => status,
    }
}

