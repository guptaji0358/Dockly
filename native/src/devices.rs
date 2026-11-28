//! Read-only device inventory: storage volumes and present Plug and Play devices.
//! No device is opened for data transfer and no elevation is needed.

pub const STORAGE: u32 = 1;
pub const USB: u32 = 2;
pub const HID: u32 = 3;
pub const SERIAL: u32 = 4;
pub const BLUETOOTH: u32 = 5;
pub const NETWORK: u32 = 6;
pub const DISPLAY: u32 = 7;
pub const SENSOR: u32 = 8;

pub const STARTED: u32 = 1;
pub const PROBLEM: u32 = 2;
pub const REMOVABLE: u32 = 4;
pub const REMOTE: u32 = 8;
pub const HAS_SIZE: u32 = 16;

pub const NAME_LEN: usize = 256;
pub const ID_LEN: usize = 512;
pub const DETAIL_LEN: usize = 128;

/// Mirrors `dn_device` in the header. Text is NUL-terminated UTF-8.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct Device {
    pub kind: u32,
    pub flags: u32,
    pub problem: u32,
    pub vendor_id: u16,
    pub product_id: u16,
    pub total_bytes: u64,
    pub free_bytes: u64,
    pub name: [u8; NAME_LEN],
    pub id: [u8; ID_LEN],
    pub detail: [u8; DETAIL_LEN],
}

impl Device {
    fn new(kind: u32) -> Self {
        Self {
            kind,
            flags: 0,
            problem: 0,
            vendor_id: 0,
            product_id: 0,
            total_bytes: 0,
            free_bytes: 0,
            name: [0; NAME_LEN],
            id: [0; ID_LEN],
            detail: [0; DETAIL_LEN],
        }
    }
}

// the same layout checks exist in scripts/src/nativebridge.cpp
const _: () = {
    use std::mem::{offset_of, size_of};
    assert!(size_of::<Device>() == 928);
    assert!(offset_of!(Device, total_bytes) == 16);
    assert!(offset_of!(Device, name) == 32);
    assert!(offset_of!(Device, id) == 288);
    assert!(offset_of!(Device, detail) == 800);
};

/// Copies `text` into a zeroed fixed field, cut at a character boundary, always NUL-terminated.
fn put(field: &mut [u8], text: &str) {
    let mut n = text.len().min(field.len() - 1);
    while !text.is_char_boundary(n) {
        n -= 1;
    }
    field[..n].copy_from_slice(&text.as_bytes()[..n]);
}

/// The text before the first NUL of a UTF-16 buffer.
fn from_wide(wide: &[u16]) -> String {
    let end = wide.iter().position(|&c| c == 0).unwrap_or(wide.len());
    String::from_utf16_lossy(&wide[..end])
}

/// Splits a double-NUL-terminated list of UTF-16 strings.
fn split_wide_list(list: &[u16]) -> impl Iterator<Item = &[u16]> {
    list.split(|&c| c == 0).take_while(|s| !s.is_empty())
}

/// The four hex digits after `key` in a device instance id such as `USB\VID_046D&PID_C52B\5&1`.
fn hex_after(id: &str, key: &str) -> u16 {
    let upper = id.to_ascii_uppercase();
    upper
        .find(key)
        .and_then(|at| upper.get(at + key.len()..at + key.len() + 4))
        .and_then(|digits| u16::from_str_radix(digits, 16).ok())
        .unwrap_or(0)
}

#[cfg(windows)]
mod os;

#[cfg(windows)]
pub use os::list;

#[cfg(not(windows))]
pub fn list(_kind: u32) -> Result<Vec<Device>, i32> {
    Err(crate::ffi::ERR_OS)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn text_is_cut_at_a_character_boundary() {
        let mut field = [0u8; 4];
        put(&mut field, "ab\u{e9}"); // the two-byte character does not fit
        assert_eq!(field, [b'a', b'b', 0, 0]);
        let mut exact = [0u8; 4];
        put(&mut exact, "abcdef");
        assert_eq!(exact, [b'a', b'b', b'c', 0]);
    }

    #[test]
    fn parses_wide_text_and_lists() {
        let list: Vec<u16> = "one\0two\0\0junk".encode_utf16().collect();
        let names: Vec<String> = split_wide_list(&list).map(from_wide).collect();
        assert_eq!(names, ["one", "two"]);
        assert_eq!(split_wide_list(&[0, 0]).count(), 0);
        assert_eq!(from_wide(&[65, 66]), "AB"); // no terminator
    }

    #[test]
    fn reads_vendor_and_product_ids() {
        let id = r"USB\VID_046D&PID_c52b\5&1a2b";
        assert_eq!((hex_after(id, "VID_"), hex_after(id, "PID_")), (0x046D, 0xC52B));
        assert_eq!(hex_after(r"USB\ROOT_HUB30\4&1", "VID_"), 0);
        assert_eq!(hex_after("VID_04", "VID_"), 0); // truncated
        assert_eq!(hex_after("VID_ZZZZ", "VID_"), 0);
    }

    #[cfg(windows)]
    #[test]
    fn lists_this_machine() {
        let volumes = list(STORAGE).expect("volumes");
        assert!(volumes.iter().any(|v| v.flags & HAS_SIZE != 0 && v.total_bytes >= v.free_bytes));
        for kind in USB..=SENSOR {
            for d in list(kind).expect("devices") {
                assert_eq!(d.kind, kind);
                assert!(d.id[0] != 0);
            }
        }
        assert!(list(0).is_err() && list(99).is_err());
    }
}
