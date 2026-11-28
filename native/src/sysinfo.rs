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

