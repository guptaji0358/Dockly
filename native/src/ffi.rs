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

