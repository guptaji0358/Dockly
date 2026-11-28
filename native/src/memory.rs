//! Memory visibility: this DLL's own heap use and a snapshot of the whole process.
//! The standard allocator is kept; it is only counted.

use std::alloc::{GlobalAlloc, Layout, System};
use std::sync::atomic::{AtomicU64, Ordering::Relaxed};

use crate::ffi::OutStruct;

/// `System` plus counters. Only allocations made by this DLL pass through it, never Qt's or C++'s.
pub struct Counting {
    live: AtomicU64,
    peak: AtomicU64,
    allocations: AtomicU64,
    frees: AtomicU64,
}

impl Counting {
    pub const fn new() -> Self {
        Self { live: AtomicU64::new(0), peak: AtomicU64::new(0), allocations: AtomicU64::new(0), frees: AtomicU64::new(0) }
    }

    fn grew(&self, by: u64) {
        let now = self.live.fetch_add(by, Relaxed) + by;
        self.peak.fetch_max(now, Relaxed);
    }

    /// (live bytes, peak bytes, allocations, frees). Not one atomic snapshot while threads allocate.
    pub fn counts(&self) -> (u64, u64, u64, u64) {
        (self.live.load(Relaxed), self.peak.load(Relaxed), self.allocations.load(Relaxed), self.frees.load(Relaxed))
    }
}

// SAFETY: every request is passed to `System` unchanged; the counters never touch the memory.
unsafe impl GlobalAlloc for Counting {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        // SAFETY: the caller's contract is `System`'s contract.
        let p = unsafe { System.alloc(layout) };
        if !p.is_null() {
            self.allocations.fetch_add(1, Relaxed);
            self.grew(layout.size() as u64);
        }
        p
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        // SAFETY: as above.
        unsafe { System.dealloc(ptr, layout) };
        self.frees.fetch_add(1, Relaxed);
        self.live.fetch_sub(layout.size() as u64, Relaxed);
    }

    unsafe fn realloc(&self, ptr: *mut u8, layout: Layout, new_size: usize) -> *mut u8 {
        // SAFETY: as above.
        let p = unsafe { System.realloc(ptr, layout, new_size) };
        if !p.is_null() {
            // the block stays one allocation; only its size changes
            let (old, new) = (layout.size() as u64, new_size as u64);
            if new >= old {
                self.grew(new - old);
            } else {
                self.live.fetch_sub(old - new, Relaxed);
            }
        }
        p
    }
}

#[global_allocator]
pub static HEAP: Counting = Counting::new();

/// Mirrors `dn_memory_info` in the header.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct MemoryInfo {
    pub struct_size: u32,
    pub handle_count: u32,
    pub working_set_bytes: u64,
    pub peak_working_set_bytes: u64,
    pub private_bytes: u64,
    pub peak_commit_bytes: u64,
    pub page_faults: u64,
    pub committed_private_bytes: u64,
    pub committed_mapped_bytes: u64,
    pub committed_image_bytes: u64,
    pub reserved_bytes: u64,
    pub free_bytes: u64,
    pub largest_free_block_bytes: u64,
    pub free_block_count: u64,
    pub native_live_bytes: u64,
    pub native_peak_bytes: u64,
    pub native_allocations: u64,
    pub native_frees: u64,
}

// SAFETY: repr(C), integers only, `struct_size` first.
unsafe impl OutStruct for MemoryInfo {
    fn set_struct_size(&mut self, size: u32) {
        self.struct_size = size;
    }
}

// the same layout checks exist in scripts/src/nativebridge.cpp
const _: () = {
    use std::mem::{offset_of, size_of};
    assert!(size_of::<MemoryInfo>() == 136);
    assert!(offset_of!(MemoryInfo, working_set_bytes) == 8);
    assert!(offset_of!(MemoryInfo, committed_private_bytes) == 48);
    assert!(offset_of!(MemoryInfo, native_frees) == 128);
};

pub const MEM_COMMIT: u32 = 0x1000;
pub const MEM_RESERVE: u32 = 0x2000;
pub const MEM_FREE: u32 = 0x1_0000;
pub const MEM_PRIVATE: u32 = 0x2_0000;
pub const MEM_MAPPED: u32 = 0x4_0000;
pub const MEM_IMAGE: u32 = 0x100_0000;

/// Adds one `VirtualQuery` region to the address space totals.
fn add_region(info: &mut MemoryInfo, state: u32, kind: u32, size: u64) {
    match state {
        MEM_COMMIT => match kind {
            MEM_PRIVATE => info.committed_private_bytes += size,
            MEM_MAPPED => info.committed_mapped_bytes += size,
            MEM_IMAGE => info.committed_image_bytes += size,
            _ => {}
        },
        MEM_RESERVE => info.reserved_bytes += size,
        MEM_FREE => {
            info.free_bytes += size;
            info.free_block_count += 1;
            info.largest_free_block_bytes = info.largest_free_block_bytes.max(size);
        }
        _ => {}
    }
}

#[cfg(windows)]
mod os {
    use super::{add_region, MemoryInfo, HEAP};
    use crate::ffi::ERR_OS;

    #[repr(C)]
    #[derive(Default)]
    struct ProcessMemoryCounters {
        cb: u32,
        page_fault_count: u32,
        peak_working_set: usize,
        working_set: usize,
        quota_peak_paged: usize,
        quota_paged: usize,
        quota_peak_non_paged: usize,
        quota_non_paged: usize,
        pagefile: usize,
        peak_pagefile: usize,
        private_usage: usize,
    }

    #[repr(C)]
    struct MemoryBasicInformation {
        base_address: usize,
        allocation_base: usize,
        allocation_protect: u32,
        partition_id: u16,
        region_size: usize,
        state: u32,
        protect: u32,
        kind: u32,
    }

    #[link(name = "kernel32")]
    extern "system" {
        fn GetCurrentProcess() -> isize;
        fn K32GetProcessMemoryInfo(process: isize, counters: *mut ProcessMemoryCounters, size: u32) -> i32;
        fn GetProcessHandleCount(process: isize, count: *mut u32) -> i32;
        fn VirtualQuery(address: usize, info: *mut MemoryBasicInformation, length: usize) -> usize;
    }

    pub fn read() -> Result<MemoryInfo, i32> {
        // taken first, so that nothing below shows up in them
        let (live, peak, allocations, frees) = HEAP.counts();
        let mut counters = ProcessMemoryCounters { cb: size_of::<ProcessMemoryCounters>() as u32, ..Default::default() };
        // SAFETY: the pseudo handle is always valid; the struct is writable and `cb` bytes long.
        if unsafe { K32GetProcessMemoryInfo(GetCurrentProcess(), &mut counters, counters.cb) } == 0 {
            return Err(ERR_OS);
        }
        let mut handles = 0;
        // SAFETY: a writable u32.
        if unsafe { GetProcessHandleCount(GetCurrentProcess(), &mut handles) } == 0 {
            return Err(ERR_OS);
        }
        let mut info = MemoryInfo {
            handle_count: handles,
            working_set_bytes: counters.working_set as u64,
            peak_working_set_bytes: counters.peak_working_set as u64,
            private_bytes: counters.private_usage as u64,
            peak_commit_bytes: counters.peak_pagefile as u64,
            page_faults: u64::from(counters.page_fault_count),
            native_live_bytes: live,
            native_peak_bytes: peak,
            native_allocations: allocations,
            native_frees: frees,
            ..Default::default()
        };
        // walk the user address space region by region; only page attributes are read
        let mut address = 0usize;
        loop {
            let mut region = std::mem::MaybeUninit::<MemoryBasicInformation>::zeroed();
            let length = size_of::<MemoryBasicInformation>();
            // SAFETY: a writable buffer of `length` bytes; `address` is never dereferenced.
            if unsafe { VirtualQuery(address, region.as_mut_ptr(), length) } == 0 {
                break; // past the last user address
            }
            // SAFETY: zero-initialised integers, then filled by Windows.
            let region = unsafe { region.assume_init() };
            add_region(&mut info, region.state, region.kind, region.region_size as u64);
            match region.base_address.checked_add(region.region_size) {
                Some(next) if next > address => address = next,
                _ => break,
            }
        }
        Ok(info)
    }
}

#[cfg(windows)]
pub use os::read;

#[cfg(not(windows))]
pub fn read() -> Result<MemoryInfo, i32> {
    Err(crate::ffi::ERR_OS)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn counts_balance_through_grow_shrink_and_free() {
        let heap = Counting::new();
        let layout = Layout::from_size_align(64, 8).unwrap();
        // SAFETY: a non-zero layout; the pointer is reallocated and freed with matching layouts.
        unsafe {
            let p = heap.alloc(layout);
            let p = heap.realloc(p, layout, 256);
            let big = Layout::from_size_align(256, 8).unwrap();
            let p = heap.realloc(p, big, 16);
            assert_eq!(heap.counts(), (16, 256, 1, 0));
            heap.dealloc(p, Layout::from_size_align(16, 8).unwrap());
        }
        assert_eq!(heap.counts(), (0, 256, 1, 1));
    }

    #[test]
    fn regions_are_classified() {
        let mut info = MemoryInfo::default();
        for (state, kind, size) in
            [(MEM_COMMIT, MEM_PRIVATE, 1), (MEM_COMMIT, MEM_MAPPED, 2), (MEM_COMMIT, MEM_IMAGE, 4), (MEM_RESERVE, MEM_PRIVATE, 8), (MEM_FREE, 0, 16), (MEM_FREE, 0, 32)]
        {
            add_region(&mut info, state, kind, size);
        }
        assert_eq!((info.committed_private_bytes, info.committed_mapped_bytes, info.committed_image_bytes), (1, 2, 4));
        assert_eq!((info.reserved_bytes, info.free_bytes, info.largest_free_block_bytes, info.free_block_count), (8, 48, 32, 2));
    }

    #[cfg(windows)]
    #[test]
    fn reads_this_process() {
        let info = read().expect("snapshot");
        assert!(info.private_bytes > 0 && info.handle_count > 0);
        assert!(info.peak_working_set_bytes >= info.working_set_bytes);
        assert!(info.committed_image_bytes > 0 && info.largest_free_block_bytes <= info.free_bytes);
    }
}
