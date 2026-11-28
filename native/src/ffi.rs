//! Boundary checks: status codes, panic containment and caller-owned output structs.

use std::panic::{catch_unwind, AssertUnwindSafe};

pub const OK: i32 = 0;
pub const ERR_ARG: i32 = -1;
pub const ERR_SIZE: i32 = -2;
pub const ERR_OS: i32 = -3;
pub const ERR_PANIC: i32 = -4;
pub const ERR_MORE: i32 = -5;
pub const ERR_LIMIT: i32 = -6;

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

/// Copies a list into a caller-owned array. `count` always receives the list length once the
/// pointers pass the checks; items are written only when all of them fit.
///
/// # Safety
/// `items` is null, or points to `capacity` writable elements of `item_size` bytes each.
/// `count` is null, or points to a writable u32.
pub unsafe fn write_list<T: Copy>(
    items: *mut T,
    capacity: u32,
    item_size: u32,
    count: *mut u32,
    produce: impl FnOnce() -> Result<Vec<T>, i32>,
) -> i32 {
    if count.is_null() || !count.is_aligned() || !items.is_aligned() || (items.is_null() && capacity != 0) {
        return ERR_ARG;
    }
    // an array has a fixed stride, so unlike a single struct the size must match exactly
    if item_size as usize != size_of::<T>() {
        return ERR_SIZE;
    }
    let found = match produce() {
        Ok(found) => found,
        Err(status) => return status,
    };
    let Ok(total) = u32::try_from(found.len()) else {
        return ERR_LIMIT;
    };
    // SAFETY: non-null and aligned (checked).
    unsafe { count.write(total) };
    if total > capacity {
        return ERR_MORE;
    }
    for (index, item) in found.into_iter().enumerate() {
        // SAFETY: index < total <= capacity, and `items` is non-null when capacity > 0.
        unsafe { items.add(index).write(item) };
    }
    OK
}

#[cfg(test)]
mod tests {
    use super::*;

    #[repr(C)]
    #[derive(Clone, Copy)]
    struct Sample {
        size: u32,
        value: u32,
    }
    // SAFETY: repr(C), integers only, size first.
    unsafe impl OutStruct for Sample {
        fn set_struct_size(&mut self, size: u32) {
            self.size = size;
        }
    }
    const FULL: u32 = size_of::<Sample>() as u32;

    fn made() -> Result<Sample, i32> {
        Ok(Sample { size: 0, value: 7 })
    }

    #[test]
    fn panic_becomes_status() {
        assert_eq!(guard(|| panic!("boom")), ERR_PANIC);
        assert_eq!(guard(|| OK), OK);
    }

    #[test]
    fn rejects_null_and_misaligned() {
        assert_eq!(unsafe { write_out::<Sample>(std::ptr::null_mut(), made) }, ERR_ARG);
        let mut words = [FULL; 4];
        let odd = words.as_mut_ptr().cast::<u8>().wrapping_add(1).cast::<Sample>();
        assert_eq!(unsafe { write_out(odd, made) }, ERR_ARG);
    }

    #[test]
    fn short_struct_is_untouched() {
        let mut s = Sample { size: FULL - 1, value: 99 };
        assert_eq!(unsafe { write_out(&mut s, made) }, ERR_SIZE);
        assert_eq!((s.size, s.value), (FULL - 1, 99));
    }

    #[test]
    fn failure_leaves_output_unchanged() {
        let mut s = Sample { size: FULL, value: 99 };
        assert_eq!(unsafe { write_out(&mut s, || Err(ERR_OS)) }, ERR_OS);
        assert_eq!(s.value, 99);
    }

    #[test]
    fn list_reports_the_count_and_writes_all_or_nothing() {
        let three = || Ok(vec![1u32, 2, 3]);
        let mut items = [9u32; 4];
        let mut count = 0;
        let none = std::ptr::null_mut::<u32>();
        // SAFETY: every pointer is null or points to the stated number of elements.
        unsafe {
            assert_eq!(write_list(none, 0, 4, &mut count, three), ERR_MORE);
            assert_eq!(count, 3);
            assert_eq!(write_list(items.as_mut_ptr(), 2, 4, &mut count, three), ERR_MORE);
            assert_eq!(items, [9; 4]);
            assert_eq!(write_list(items.as_mut_ptr(), 4, 4, &mut count, three), OK);
            assert_eq!(items, [1, 2, 3, 9]);
            assert_eq!(write_list(items.as_mut_ptr(), 4, 8, &mut count, three), ERR_SIZE);
            assert_eq!(write_list(none, 4, 4, &mut count, three), ERR_ARG);
            assert_eq!(write_list(items.as_mut_ptr(), 4, 4, none, three), ERR_ARG);
            assert_eq!(write_list(none, 0, 4, &mut count, || Ok(Vec::new())), OK);
            assert_eq!(count, 0);
        }
    }

    #[test]
    fn larger_caller_keeps_its_size_and_tail() {
        let mut words = [FULL + 8, 0, 0xAAAA_AAAA, 0xBBBB_BBBB];
        assert_eq!(unsafe { write_out(words.as_mut_ptr().cast::<Sample>(), made) }, OK);
        assert_eq!(words, [FULL + 8, 7, 0xAAAA_AAAA, 0xBBBB_BBBB]);
    }
}
