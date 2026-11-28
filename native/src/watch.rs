//! Device connect and disconnect events. Windows calls back on its own thread; the callback only
//! counts, and the caller polls the counts, so nothing calls back into C++.
//!
//! Handles are numbers looked up in a table, never pointers, so a closed or made-up handle is
//! refused instead of touching freed memory.

use std::collections::BTreeMap;
use std::sync::atomic::{AtomicU64, Ordering::Relaxed};
use std::sync::{Mutex, MutexGuard};

use crate::ffi::{OutStruct, ERR_ARG, ERR_LIMIT};

const ACTION_INSTANCE_STARTED: u32 = 8;
const ACTION_INSTANCE_REMOVED: u32 = 9;
const MAX_OPEN: usize = 64;

#[repr(C)]
#[derive(Default)]
struct Counters {
    arrivals: AtomicU64,
    removals: AtomicU64,
}

impl Counters {
    fn record(&self, action: u32) {
        match action {
            ACTION_INSTANCE_STARTED => self.arrivals.fetch_add(1, Relaxed),
            ACTION_INSTANCE_REMOVED => self.removals.fetch_add(1, Relaxed),
            _ => 0,
        };
    }
}

/// Mirrors `dn_watch_counts` in the header.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct WatchCounts {
    pub struct_size: u32,
    pub reserved: u32,
    pub arrivals: u64,
    pub removals: u64,
}

// SAFETY: repr(C), integers only, `struct_size` first.
unsafe impl OutStruct for WatchCounts {
    fn set_struct_size(&mut self, size: u32) {
        self.struct_size = size;
    }
}

// the same layout checks exist in scripts/src/nativebridge.cpp
const _: () = {
    use std::mem::{offset_of, size_of};
    assert!(size_of::<WatchCounts>() == 24);
    assert!(offset_of!(WatchCounts, arrivals) == 8);
};

#[cfg(windows)]
mod os {
    use super::Counters;
    use crate::ffi::ERR_OS;

    const FILTER_DEVICE_INSTANCE: u32 = 2;
    const FLAG_ALL_DEVICE_INSTANCES: u32 = 2;

    /// `CM_NOTIFY_FILTER`; the union is as large as its biggest member, a 200 character id.
    #[repr(C)]
    struct Filter {
        size: u32,
        flags: u32,
        filter_type: u32,
        reserved: u32,
        instance_id: [u16; 200],
    }
    const _: () = assert!(size_of::<Filter>() == 416);

    type Callback = unsafe extern "system" fn(isize, *const Counters, u32, *const u8, u32) -> u32;

    #[link(name = "cfgmgr32")]
    extern "system" {
        fn CM_Register_Notification(
            filter: *const Filter,
            context: *const Counters,
            callback: Callback,
            handle: *mut isize,
        ) -> u32;
        fn CM_Unregister_Notification(handle: isize) -> u32;
    }

    unsafe extern "system" fn on_event(_: isize, context: *const Counters, action: u32, _: *const u8, _: u32) -> u32 {
        // SAFETY: `context` is the boxed `Counters` of a live `Watch`: unregistering, which waits
        // for running callbacks, happens before the box is freed. Only atomics are touched.
        if let Some(counters) = unsafe { context.as_ref() } {
            counters.record(action);
        }
        0 // ERROR_SUCCESS
    }

    pub struct Watch {
        notification: isize,
        counters: *mut Counters,
    }

    // SAFETY: the notification handle may be released from any thread, and the counters are atomics.
    unsafe impl Send for Watch {}

    impl Watch {
        pub fn open() -> Result<Self, i32> {
            let filter = Filter {
                size: size_of::<Filter>() as u32,
                flags: FLAG_ALL_DEVICE_INSTANCES,
                filter_type: FILTER_DEVICE_INSTANCE,
                reserved: 0,
                instance_id: [0; 200],
            };
            let counters = Box::into_raw(Box::<Counters>::default());
            let mut notification = 0;
            // SAFETY: a correctly sized filter, a context that outlives the registration, and a
            // writable handle.
            let status = unsafe { CM_Register_Notification(&filter, counters, on_event, &mut notification) };
            if status != 0 {
                // SAFETY: registration failed, so nothing else holds the pointer.
                drop(unsafe { Box::from_raw(counters) });
                return Err(ERR_OS);
            }
            Ok(Self { notification, counters })
        }

        pub(super) fn counters(&self) -> &Counters {
            // SAFETY: allocated in `open`, freed only in `drop`.
            unsafe { &*self.counters }
        }
    }

    impl Drop for Watch {
        fn drop(&mut self) {
            // SAFETY: a registered handle, released once, never from inside the callback.
            // Waits until no callback is running.
            if unsafe { CM_Unregister_Notification(self.notification) } == 0 {
                // SAFETY: no callback can use the counters any more.
                drop(unsafe { Box::from_raw(self.counters) });
            } // else: leak 24 bytes rather than free memory Windows might still use
        }
    }
}

#[cfg(not(windows))]
mod os {
    pub struct Watch(super::Counters);
    impl Watch {
        pub fn open() -> Result<Self, i32> {
            Err(crate::ffi::ERR_OS)
        }
        pub(super) fn counters(&self) -> &super::Counters {
            &self.0
        }
    }
}

struct Table {
    next: u64,
    open: BTreeMap<u64, os::Watch>,
}

static TABLE: Mutex<Table> = Mutex::new(Table { next: 1, open: BTreeMap::new() });

fn table() -> MutexGuard<'static, Table> {
    // the table stays consistent even if a holder panicked
    TABLE.lock().unwrap_or_else(|poisoned| poisoned.into_inner())
}

/// Starts watching. The handle is never 0 and is never reused.
pub fn open() -> Result<u64, i32> {
    let watch = os::Watch::open()?;
    let mut table = table();
    if table.open.len() >= MAX_OPEN {
        return Err(ERR_LIMIT); // `watch` is dropped after the guard
    }
    let handle = table.next;
    table.next += 1;
    table.open.insert(handle, watch);
    Ok(handle)
}

/// Totals since `open`.
pub fn counts(handle: u64) -> Result<WatchCounts, i32> {
    let table = table();
    let counters = table.open.get(&handle).ok_or(ERR_ARG)?.counters();
    Ok(WatchCounts {
        arrivals: counters.arrivals.load(Relaxed),
        removals: counters.removals.load(Relaxed),
        ..Default::default()
    })
}

pub fn close(handle: u64) -> Result<(), i32> {
    // unregister after the lock is released
    let watch = table().open.remove(&handle);
    watch.map(drop).ok_or(ERR_ARG)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn only_start_and_remove_are_counted() {
        let c = Counters::default();
        for action in [7, 8, 8, 9, 0, 42] {
            c.record(action);
        }
        assert_eq!((c.arrivals.load(Relaxed), c.removals.load(Relaxed)), (2, 1));
    }

    #[cfg(windows)]
    #[test]
    fn handles_are_checked() {
        assert_eq!(close(0), Err(ERR_ARG));
        let a = open().expect("watch");
        let b = open().expect("watch");
        assert!(a != 0 && a != b);
        assert!(counts(a).is_ok());
        assert_eq!(close(a), Ok(()));
        assert_eq!(close(a), Err(ERR_ARG));
        assert_eq!(counts(a).map(|_| ()), Err(ERR_ARG));
        assert_eq!(close(b), Ok(()));
    }
}
