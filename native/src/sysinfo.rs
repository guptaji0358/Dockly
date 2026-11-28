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

    pub fn read() -> Result<SystemInfo, i32> {
        let mut memory = MemoryStatusEx { length: size_of::<MemoryStatusEx>() as u32, ..Default::default() };
        // SAFETY: a correctly sized, writable MEMORYSTATUSEX.
        if unsafe { GlobalMemoryStatusEx(&mut memory) } == 0 {
            return Err(ERR_OS);
        }
        let mut version = OsVersionInfo {
            size: size_of::<OsVersionInfo>() as u32,
            major: 0,
            minor: 0,
            build: 0,
            platform: 0,
            service_pack: [0; 128],
        };
        // SAFETY: a correctly sized, writable OSVERSIONINFOW.
        if unsafe { RtlGetVersion(&mut version) } != 0 {
            return Err(ERR_OS);
        }
        // SAFETY: takes a plain value.
        let logical = unsafe { GetActiveProcessorCount(ALL_PROCESSOR_GROUPS) };
        if logical == 0 {
            return Err(ERR_OS);
        }
        let (cores, packages) = cores_and_packages()?;
        Ok(SystemInfo {
            struct_size: 0, // set by ffi::write_out
            logical_processors: logical,
            physical_cores: cores,
            processor_packages: packages,
            total_physical_bytes: memory.total_phys,
            available_physical_bytes: memory.avail_phys,
            os_major: version.major,
            os_minor: version.minor,
            os_build: version.build,
            memory_load_percent: memory.memory_load,
        })
    }
}

#[cfg(windows)]
pub use os::read;

#[cfg(not(windows))]
pub fn read() -> Result<SystemInfo, i32> {
    Err(crate::ffi::ERR_OS)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn record(relation: u32, size: u32) -> Vec<u8> {
        let mut r = vec![0u8; size.max(8) as usize];
        r[0..4].copy_from_slice(&relation.to_ne_bytes());
        r[4..8].copy_from_slice(&size.to_ne_bytes());
        r
    }

    #[test]
    fn counts_cores_and_packages() {
        let buf = [record(3, 48), record(0, 48), record(2, 64), record(0, 32)].concat();
        assert_eq!(count_relations(&buf), Some((2, 1)));
        assert_eq!(count_relations(&[]), Some((0, 0)));
    }

    #[test]
    fn rejects_malformed_buffers() {
        assert_eq!(count_relations(&record(0, 0)), None); // zero size
        assert_eq!(count_relations(&record(0, 48)[..40]), None); // truncated record
        assert_eq!(count_relations(&[0, 0, 0]), None); // truncated header
    }

    #[cfg(windows)]
    #[test]
    fn reads_this_machine() {
        let info = read().expect("snapshot");
        assert!(info.physical_cores >= 1 && info.logical_processors >= info.physical_cores);
        assert!(info.processor_packages >= 1);
        assert!(info.total_physical_bytes >= info.available_physical_bytes);
        assert!(info.memory_load_percent <= 100 && info.os_major >= 10);
    }
}
