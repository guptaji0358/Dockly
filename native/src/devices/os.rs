//! The Windows calls behind the device inventory: volume APIs and the Configuration Manager.

use super::*;
use crate::ffi::{ERR_ARG, ERR_OS};

const CR_SUCCESS: u32 = 0;
const CR_BUFFER_SMALL: u32 = 0x1A;
const FILTER_ENUMERATOR: u32 = 0x1;
const FILTER_PRESENT: u32 = 0x100;
const FILTER_CLASS: u32 = 0x200;
const DN_STARTED: u32 = 0x8;
const DN_HAS_PROBLEM: u32 = 0x400;
const PROPERTY_DESCRIPTION: u32 = 0x01;
const PROPERTY_MANUFACTURER: u32 = 0x0C;
const PROPERTY_FRIENDLY_NAME: u32 = 0x0D;

const DRIVE_REMOVABLE: u32 = 2;
const DRIVE_REMOTE: u32 = 4;
const DRIVE_CDROM: u32 = 5;
const SEM_FAILCRITICALERRORS: u32 = 1;

#[link(name = "cfgmgr32")]
extern "system" {
    fn CM_Get_Device_ID_List_SizeW(length: *mut u32, filter: *const u16, flags: u32) -> u32;
    fn CM_Get_Device_ID_ListW(filter: *const u16, buffer: *mut u16, length: u32, flags: u32) -> u32;
    fn CM_Locate_DevNodeW(node: *mut u32, id: *const u16, flags: u32) -> u32;
    fn CM_Get_DevNode_Status(status: *mut u32, problem: *mut u32, node: u32, flags: u32) -> u32;
    fn CM_Get_DevNode_Registry_PropertyW(
        node: u32,
        property: u32,
        kind: *mut u32,
        buffer: *mut u16,
        bytes: *mut u32,
        flags: u32,
    ) -> u32;
}

#[link(name = "kernel32")]
extern "system" {
    fn GetLogicalDrives() -> u32;
    fn GetDriveTypeW(root: *const u16) -> u32;
    fn GetVolumeInformationW(
        root: *const u16,
        name: *mut u16,
        name_length: u32,
        serial: *mut u32,
        component_length: *mut u32,
        flags: *mut u32,
        file_system: *mut u16,
        file_system_length: u32,
    ) -> i32;
    fn GetDiskFreeSpaceExW(root: *const u16, available: *mut u64, total: *mut u64, free: *mut u64) -> i32;
    fn SetThreadErrorMode(mode: u32, old: *mut u32) -> i32;
}

fn wide(text: &str) -> Vec<u16> {
    text.encode_utf16().chain([0]).collect()
}

pub fn list(kind: u32) -> Result<Vec<Device>, i32> {
    match kind {
        STORAGE => volumes(),
        USB => present(kind, "USB", FILTER_ENUMERATOR),
        HID => present(kind, "HID", FILTER_ENUMERATOR),
        SERIAL => present(kind, "{4d36e978-e325-11ce-bfc1-08002be10318}", FILTER_CLASS),
        BLUETOOTH => present(kind, "{e0cbf06c-cd8b-4647-bb8a-263b43f0f974}", FILTER_CLASS),
        NETWORK => present(kind, "{4d36e972-e325-11ce-bfc1-08002be10318}", FILTER_CLASS),
        DISPLAY => present(kind, "{4d36e968-e325-11ce-bfc1-08002be10318}", FILTER_CLASS),
        SENSOR => present(kind, "{5175d334-c371-4806-b3ba-71fd53c9258d}", FILTER_CLASS),
        _ => Err(ERR_ARG),
    }
}

/// Local, removable, optical and mapped drives. Sizes of remote drives are not queried, because an
/// unreachable server would block the call.
fn volumes() -> Result<Vec<Device>, i32> {
    // SAFETY: no arguments.
    let mask = unsafe { GetLogicalDrives() };
    if mask == 0 {
        return Err(ERR_OS);
    }
    let mut old_mode = 0;
    // SAFETY: a writable u32. Stops "insert a disk" dialogs for empty drives, on this thread only.
    unsafe { SetThreadErrorMode(SEM_FAILCRITICALERRORS, &mut old_mode) };
    let found = (0..26u8).filter(|bit| mask & (1 << bit) != 0).map(|bit| volume(char::from(b'A' + bit))).collect();
    // SAFETY: restores the value read above; the second pointer may be null.
    unsafe { SetThreadErrorMode(old_mode, std::ptr::null_mut()) };
    Ok(found)
}

fn volume(letter: char) -> Device {
    let path = format!("{letter}:\\");
    let root = wide(&path);
    let mut device = Device::new(STORAGE);
    put(&mut device.id, &path);
    put(&mut device.name, &path);
    // SAFETY: a NUL-terminated string.
    let drive_type = unsafe { GetDriveTypeW(root.as_ptr()) };
    match drive_type {
        DRIVE_REMOVABLE | DRIVE_CDROM => device.flags |= REMOVABLE,
        DRIVE_REMOTE => {
            device.flags |= REMOTE;
            return device;
        }
        _ => {}
    }
    let (mut label, mut file_system) = ([0u16; 261], [0u16; 261]);
    let none = std::ptr::null_mut();
    // SAFETY: both buffers are writable for the lengths given; unused out-pointers may be null.
    let ready = unsafe {
        GetVolumeInformationW(root.as_ptr(), label.as_mut_ptr(), 261, none, none, none, file_system.as_mut_ptr(), 261)
    } != 0;
    if !ready {
        return device; // no media, or not readable
    }
    device.flags |= STARTED;
    let label = from_wide(&label);
    if !label.is_empty() {
        device.name = [0; NAME_LEN];
        put(&mut device.name, &label);
    }
    put(&mut device.detail, &from_wide(&file_system));
    let (mut total, mut free) = (0, 0);
    // SAFETY: writable u64s; the first out-pointer may be null.
    if unsafe { GetDiskFreeSpaceExW(root.as_ptr(), std::ptr::null_mut(), &mut total, &mut free) } != 0 {
        device.flags |= HAS_SIZE;
        device.total_bytes = total;
        device.free_bytes = free;
    }
    device
}

/// Instance ids of the present devices that match the filter.
fn id_list(filter: &[u16], flags: u32) -> Result<Vec<u16>, i32> {
    // retried because a device can arrive between the two calls
    for _ in 0..4 {
        let mut length = 0;
        // SAFETY: a writable u32 and a NUL-terminated filter.
        if unsafe { CM_Get_Device_ID_List_SizeW(&mut length, filter.as_ptr(), flags) } != CR_SUCCESS {
            return Err(ERR_OS);
        }
        let mut buffer = vec![0u16; length.max(1) as usize];
        // SAFETY: the buffer holds the number of characters passed.
        match unsafe { CM_Get_Device_ID_ListW(filter.as_ptr(), buffer.as_mut_ptr(), buffer.len() as u32, flags) } {
            CR_SUCCESS => return Ok(buffer),
            CR_BUFFER_SMALL => continue,
            _ => return Err(ERR_OS),
        }
    }
    Err(ERR_OS)
}

fn property(node: u32, which: u32) -> String {
    let mut buffer = [0u16; 512];
    let mut bytes = size_of_val(&buffer) as u32;
    // SAFETY: the buffer is writable for `bytes` bytes; the type pointer may be null.
    let status = unsafe {
        CM_Get_DevNode_Registry_PropertyW(node, which, std::ptr::null_mut(), buffer.as_mut_ptr(), &mut bytes, 0)
    };
    if status == CR_SUCCESS {
        from_wide(&buffer)
    } else {
        String::new() // the property is optional
    }
}

fn present(kind: u32, filter: &str, how: u32) -> Result<Vec<Device>, i32> {
    let ids = id_list(&wide(filter), how | FILTER_PRESENT)?;
    let mut found = Vec::new();
    for id in split_wide_list(&ids) {
        let terminated: Vec<u16> = id.iter().copied().chain([0]).collect();
        let mut node = 0;
        // SAFETY: a writable u32 and a NUL-terminated id.
        if unsafe { CM_Locate_DevNodeW(&mut node, terminated.as_ptr(), 0) } != CR_SUCCESS {
            continue; // unplugged since the list was made
        }
        let mut name = property(node, PROPERTY_FRIENDLY_NAME);
        if name.is_empty() {
            name = property(node, PROPERTY_DESCRIPTION);
        }
        // the Ports class also holds parallel ports
        if kind == SERIAL && !name.contains("(COM") {
            continue;
        }
        let text = from_wide(id);
        let mut device = Device::new(kind);
        device.vendor_id = hex_after(&text, "VID_");
        device.product_id = hex_after(&text, "PID_");
        let (mut status, mut problem) = (0, 0);
        // SAFETY: two writable u32s.
        if unsafe { CM_Get_DevNode_Status(&mut status, &mut problem, node, 0) } == CR_SUCCESS {
            if status & DN_STARTED != 0 {
                device.flags |= STARTED;
            }
            if status & DN_HAS_PROBLEM != 0 {
                device.flags |= PROBLEM;
                device.problem = problem;
            }
        }
        put(&mut device.id, &text);
        put(&mut device.name, &name);
        put(&mut device.detail, &property(node, PROPERTY_MANUFACTURER));
        found.push(device);
    }
    Ok(found)
}
