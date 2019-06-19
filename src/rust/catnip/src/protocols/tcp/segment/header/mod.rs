#![allow(dead_code)]

#[cfg(test)]
mod tests;

use crate::prelude::*;
use byteorder::{ByteOrder, NetworkEndian};
use std::convert::TryFrom;

pub const MIN_TCP_HEADER_SIZE: usize = 20;
pub const MAX_TCP_HEADER_SIZE: usize = 60;

pub struct TcpHeader<'a>(&'a [u8]);

impl<'a> TcpHeader<'a> {
    pub fn new(bytes: &'a [u8]) -> TcpHeader<'a> {
        assert!(bytes.len() >= MIN_TCP_HEADER_SIZE);
        assert!(bytes.len() <= MAX_TCP_HEADER_SIZE);
        TcpHeader(bytes)
    }

    pub fn as_bytes(&self) -> &[u8] {
        self.0
    }

    pub fn src_port(&self) -> u16 {
        NetworkEndian::read_u16(&self.0[..2])
    }

    pub fn dest_port(&self) -> u16 {
        NetworkEndian::read_u16(&self.0[2..4])
    }

    pub fn seq_num(&self) -> u32 {
        NetworkEndian::read_u32(&self.0[4..8])
    }

    pub fn ack_num(&self) -> u32 {
        NetworkEndian::read_u32(&self.0[8..12])
    }

    pub fn header_len(&self) -> usize {
        let n = usize::from(self.0[12] >> 4);
        n * 4
    }

    pub fn ns(&self) -> bool {
        (self.0[12] & 1) != 0u8
    }

    pub fn cwr(&self) -> bool {
        (self.0[13] & (1 << 7)) != 0u8
    }

    pub fn ece(&self) -> bool {
        (self.0[13] & (1 << 6)) != 0u8
    }

    pub fn urg(&self) -> bool {
        (self.0[13] & (1 << 5)) != 0u8
    }

    pub fn ack(&self) -> bool {
        (self.0[13] & (1 << 4)) != 0u8
    }

    pub fn psh(&self) -> bool {
        (self.0[13] & (1 << 3)) != 0u8
    }

    pub fn rst(&self) -> bool {
        (self.0[13] & (1 << 2)) != 0u8
    }

    pub fn syn(&self) -> bool {
        (self.0[13] & (1 << 1)) != 0u8
    }

    pub fn fin(&self) -> bool {
        (self.0[13] & 1) != 0u8
    }

    pub fn window_sz(&self) -> u16 {
        NetworkEndian::read_u16(&self.0[14..16])
    }

    pub fn checksum(&self) -> u16 {
        NetworkEndian::read_u16(&self.0[16..18])
    }

    pub fn urg_ptr(&self) -> u16 {
        NetworkEndian::read_u16(&self.0[18..20])
    }
}

pub struct TcpHeaderMut<'a>(&'a mut [u8]);

impl<'a> TcpHeaderMut<'a> {
    pub fn new(bytes: &'a mut [u8]) -> TcpHeaderMut<'a> {
        // validate `bytes` without duplicating code.
        let _ = TcpHeader::new(bytes);
        TcpHeaderMut(bytes)
    }

    pub fn as_bytes(&mut self) -> &mut [u8] {
        self.0
    }

    pub fn src_port(&mut self, value: u16) {
        NetworkEndian::write_u16(&mut self.0[..2], value)
    }

    pub fn dest_port(&mut self, value: u16) {
        NetworkEndian::write_u16(&mut self.0[2..4], value)
    }

    pub fn seq_num(&mut self, value: u32) {
        NetworkEndian::write_u32(&mut self.0[4..8], value)
    }

    pub fn ack_num(&mut self, value: u32) {
        NetworkEndian::write_u32(&mut self.0[8..12], value)
    }

    pub fn header_len(&mut self, value: usize) -> Result<()> {
        // from [wikipedia](https://en.wikipedia.org/wiki/Transmission_Control_Protocol#TCP_segment_structure)
        // > Specifies the size of the TCP header in 32-bit words. The
        // > minimum size header is 5 words and the maximum is 15 words thus
        // > giving the minimum size of 20 bytes and maximum of 60 bytes,
        // > allowing for up to 40 bytes of options in the header.
        if value < MIN_TCP_HEADER_SIZE {
            return Err(Fail::OutOfRange {});
        }

        if value > MAX_TCP_HEADER_SIZE {
            return Err(Fail::OutOfRange {});
        }

        let mut n = value / 4;
        if n * 4 != value {
            n += 1
        }

        let n = u8::try_from(n).unwrap();
        self.0[12] = (self.0[12] & 0xf0) | (n << 4);
        Ok(())
    }

    pub fn ns(&mut self, value: bool) {
        if value {
            self.0[12] |= 1u8;
        } else {
            self.0[12] &= !1u8;
        }
    }

    pub fn cwr(&mut self, value: bool) {
        if value {
            self.0[13] |= 1u8 << 7;
        } else {
            self.0[13] &= !(1u8 << 7);
        }
    }

    pub fn ece(&mut self, value: bool) {
        if value {
            self.0[13] |= 1u8 << 6;
        } else {
            self.0[13] &= !(1u8 << 6);
        }
    }

    pub fn urg(&mut self, value: bool) {
        if value {
            self.0[13] |= 1u8 << 5;
        } else {
            self.0[13] &= !(1u8 << 5);
        }
    }

    pub fn ack(&mut self, value: bool) {
        if value {
            self.0[13] |= 1u8 << 4;
        } else {
            self.0[13] &= !(1u8 << 4);
        }
    }

    pub fn psh(&mut self, value: bool) {
        if value {
            self.0[13] |= 1u8 << 3;
        } else {
            self.0[13] &= !(1u8 << 3);
        }
    }

    pub fn rst(&mut self, value: bool) {
        if value {
            self.0[13] |= 1u8 << 2;
        } else {
            self.0[13] &= !(1u8 << 2);
        }
    }

    pub fn syn(&mut self, value: bool) {
        if value {
            self.0[13] |= 1u8 << 1;
        } else {
            self.0[13] &= !(1u8 << 1);
        }
    }

    pub fn fin(&mut self, value: bool) {
        if value {
            self.0[13] |= 1u8;
        } else {
            self.0[13] &= !1u8;
        }
    }

    pub fn window_sz(&mut self, value: u16) {
        NetworkEndian::write_u16(&mut self.0[14..16], value)
    }

    pub fn checksum(&mut self, value: u16) {
        NetworkEndian::write_u16(&mut self.0[16..18], value)
    }

    pub fn urg_ptr(&mut self, value: u16) {
        NetworkEndian::write_u16(&mut self.0[18..20], value)
    }
}
