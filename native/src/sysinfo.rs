//! Read-only system snapshot: processors, memory, Windows version. No elevation needed.

use crate::ffi::OutStruct;

/// Mirrors `dn_system_info` in the header.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct SystemInfo {
    pub struct_size: u32,
    pub logical_processors: u32,
    pub physical_cores: u32,
    pub processor_packages: u32,
    pub total_physical_bytes: u64,
    pub available_physical_bytes: u64,
    pub os_major: u32,
    pub os_minor: u32,
    pub os_build: u32,
    pub memory_load_percent: u32,
}

// SAFETY: repr(C), integers only, `struct_size` first.
unsafe impl OutStruct for SystemInfo {
    fn set_struct_size(&mut self, size: u32) {
        self.struct_size = size;
    }
}

// the same layout checks exist in scripts/src/nativebridge.cpp
const _: () = {
    use std::mem::{offset_of, size_of};
    assert!(size_of::<SystemInfo>() == 48);
    assert!(offset_of!(SystemInfo, total_physical_bytes) == 16);
    assert!(offset_of!(SystemInfo, os_major) == 32);
    assert!(offset_of!(SystemInfo, memory_load_percent) == 44);
};

const RELATION_PROCESSOR_CORE: u32 = 0;
const RELATION_PROCESSOR_PACKAGE: u32 = 3;

/// Counts (cores, packages) in a `GetLogicalProcessorInformationEx` buffer: a run of records,
/// each starting with `u32 relationship, u32 size`. `None` when the buffer is malformed.
fn count_relations(mut bytes: &[u8]) -> Option<(u32, u32)> {
    let (mut cores, mut packages) = (0, 0);
    while !bytes.is_empty() {
        let relation = u32::from_ne_bytes(bytes.get(0..4)?.try_into().ok()?);
        let size = u32::from_ne_bytes(bytes.get(4..8)?.try_into().ok()?) as usize;
        if size < 8 {
            return None; // would never advance
        }
        bytes = bytes.get(size..)?;
        match relation {
            RELATION_PROCESSOR_CORE => cores += 1,
            RELATION_PROCESSOR_PACKAGE => packages += 1,
            _ => {}
        }
    }
    Some((cores, packages))
}

#[cfg(windows)]
mod os {
    use super::{count_relations, SystemInfo};
    use crate::ffi::ERR_OS;

    const ALL_PROCESSOR_GROUPS: u16 = 0xffff;
    const RELATION_ALL: i32 = 0xffff;
    const ERROR_INSUFFICIENT_BUFFER: u32 = 122;

    #[repr(C)]
    #[derive(Default)]
    struct MemoryStatusEx {
        length: u32,
        memory_load: u32,
        total_phys: u64,
        avail_phys: u64,
        total_page_file: u64,
        avail_page_file: u64,
        total_virtual: u64,
        avail_virtual: u64,
        avail_extended_virtual: u64,
    }

    #[repr(C)]
    struct OsVersionInfo {
        size: u32,
        major: u32,
        minor: u32,
        build: u32,
        platform: u32,
        service_pack: [u16; 128],
    }

    #[link(name = "kernel32")]
    extern "system" {
        fn GlobalMemoryStatusEx(status: *mut MemoryStatusEx) -> i32;
        fn GetActiveProcessorCount(group: u16) -> u32;
        fn GetLogicalProcessorInformationEx(relation: i32, buffer: *mut u8, length: *mut u32) -> i32;
        fn GetLastError() -> u32;
    }

    #[link(name = "ntdll")]
    extern "system" {
        // unlike GetVersionExW, reports the real version without an application manifest
        fn RtlGetVersion(info: *mut OsVersionInfo) -> i32;
    }

    fn cores_and_packages() -> Result<(u32, u32), i32> {
        let mut length: u32 = 0;
        // u64 storage keeps the records 8-byte aligned, as Windows expects
        let mut buffer: Vec<u64> = Vec::new();
        // the first pass asks for the size; retry in case processors are added in between
        for _ in 0..4 {
            let data = if buffer.is_empty() { std::ptr::null_mut() } else { buffer.as_mut_ptr().cast::<u8>() };
            // SAFETY: `data` is null with length 0, or has at least `length` writable bytes.
            if unsafe { GetLogicalProcessorInformationEx(RELATION_ALL, data, &mut length) } != 0 {
                if length as usize > buffer.len() * 8 {
                    return Err(ERR_OS);
                }
                // SAFETY: the buffer is initialised and holds at least `length` bytes (checked).
                let bytes = unsafe { std::slice::from_raw_parts(data, length as usize) };
                return count_relations(bytes).ok_or(ERR_OS);
            }
            // SAFETY: no arguments.
            if unsafe { GetLastError() } != ERROR_INSUFFICIENT_BUFFER {
                break;
            }
            buffer = vec![0; (length as usize).div_ceil(8)];
            length = (buffer.len() * 8) as u32;
        }
        Err(ERR_OS)
    }

}

