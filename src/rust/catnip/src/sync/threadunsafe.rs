#![allow(unused)]
use std::{
    cell::UnsafeCell,
    fmt,
    mem,
    ops::{
        Deref,
        DerefMut,
    },
    rc::Rc,
    task::Waker,
};

struct WakerSlot(UnsafeCell<Option<Waker>>);

unsafe impl Send for WakerSlot {}
unsafe impl Sync for WakerSlot {}

pub struct SharedWaker(Rc<WakerSlot>);

impl Clone for SharedWaker {
    fn clone(&self) -> Self {
        Self(self.0.clone())
    }
}

impl SharedWaker {
    pub fn new() -> Self {
        Self(Rc::new(WakerSlot(UnsafeCell::new(None))))
    }

    pub fn register(&self, waker: &Waker) {
        let s = unsafe {
            let waker = &self.0;
            let cell = &waker.0;
            &mut *cell.get()
        };
        if let Some(ref existing_waker) = s {
            if waker.will_wake(existing_waker) {
                return;
            }
        }
        *s = Some(waker.clone());
    }

    pub fn wake(&self) {
        let s = unsafe {
            let waker = &self.0;
            let cell = &waker.0;
            &mut *cell.get()
        };
        if let Some(waker) = s.take() {
            waker.wake();
        }
    }
}

pub struct WakerU64(UnsafeCell<u64>);

unsafe impl Sync for WakerU64 {}

impl WakerU64 {
    pub fn new(val: u64) -> Self {
        WakerU64(UnsafeCell::new(val))
    }

    pub fn fetch_or(&self, val: u64) {
        let s = unsafe { &mut *self.0.get() };
        *s |= val;
    }

    pub fn fetch_and(&self, val: u64) {
        let s = unsafe { &mut *self.0.get() };
        *s &= val;
    }

    pub fn fetch_add(&self, val: u64) -> u64 {
        let s = unsafe { &mut *self.0.get() };
        let old = *s;
        *s += val;
        old
    }

    pub fn fetch_sub(&self, val: u64) -> u64 {
        let s = unsafe { &mut *self.0.get() };
        let old = *s;
        *s -= val;
        old
    }

    pub fn load(&self) -> u64 {
        let s = unsafe { &mut *self.0.get() };
        *s
    }

    pub fn swap(&self, val: u64) -> u64 {
        let s = unsafe { &mut *self.0.get() };
        mem::replace(s, val)
    }
}

use dpdk_rs::{
    rte_mempool,
    rte_mbuf,
    rte_mbuf_refcnt_update,
    rte_pktmbuf_free,
    rte_mbuf_refcnt_read,
    rte_pktmbuf_adj,
    rte_pktmbuf_trim,
};
use std::slice;
use std::ptr;

#[derive(Debug)]
pub struct Mbuf {
    pub pkt: *mut rte_mbuf,
}

impl Mbuf {
    pub fn new(pkt: *mut rte_mbuf) -> Self {
        Self { pkt }
    }

    pub fn adjust(&mut self, bytes: usize) {
        assert!(bytes <= self.len());
        assert_ne!(unsafe { rte_pktmbuf_adj(self.pkt, bytes as u16) }, ptr::null_mut());
    }

    pub fn trim(&mut self, bytes: usize) {
        assert!(bytes <= self.len());
        assert_eq!(unsafe { rte_pktmbuf_trim(self.pkt, bytes as u16) }, 0);
    }
}   

impl Deref for Mbuf {
    type Target = [u8];
    fn deref(&self) -> &[u8] {
        let rte_pktmbuf_headroom = 128;
        let buf_len = unsafe { (*self.pkt).buf_len - rte_pktmbuf_headroom };
        let ptr = unsafe { ((*self.pkt).buf_addr as *mut u8).offset((*self.pkt).data_off as isize) };
        unsafe { slice::from_raw_parts(ptr, buf_len as usize) }
    }
}

impl DerefMut for Mbuf {
    fn deref_mut(&mut self) -> &mut [u8] {
        let rte_pktmbuf_headroom = 128;
        let buf_len = unsafe { (*self.pkt).buf_len - rte_pktmbuf_headroom };
        let ptr = unsafe { ((*self.pkt).buf_addr as *mut u8).offset((*self.pkt).data_off as isize) };
        unsafe { slice::from_raw_parts_mut(ptr, buf_len as usize) }

    }
}   

impl Clone for Mbuf {
    fn clone(&self) -> Self {
        unsafe { rte_mbuf_refcnt_update(self.pkt, 1) };
        Self { pkt: self.pkt }
    }
}

impl Drop for Mbuf {
    fn drop(&mut self) {
        unsafe { rte_pktmbuf_free(self.pkt); }
        self.pkt = ptr::null_mut();
    }
}

impl Mbuf {
    fn refcount(&self) -> usize {
        let cnt = unsafe { rte_mbuf_refcnt_read(self.pkt) };
        cnt as usize
    }
}


#[derive(Clone, Debug)]
pub enum BufEnum {
    Rust(Rc<[u8]>),
    DPDK(Mbuf),
}

impl Deref for BufEnum {
    type Target = [u8];
    fn deref(&self) -> &[u8] {
        match self {
            BufEnum::Rust(ref b) => b.deref(),
            BufEnum::DPDK(ref b) => b.deref(),
        }
    }
}

impl BufEnum {
    fn refcount(&self) -> usize {
        match self {
            BufEnum::Rust(ref b) => Rc::strong_count(b),
            BufEnum::DPDK(ref b) => b.refcount(),
        }
    }
}

pub struct Bytes {
    buf: Option<BufEnum>,
    offset: usize,
    len: usize,
}

impl Clone for Bytes {
    fn clone(&self) -> Self {
        Self {
            buf: self.buf.clone(), 
            offset: self.offset,
            len: self.len,
        }
    }
}

impl PartialEq for Bytes {
    fn eq(&self, rhs: &Self) -> bool {
        &self[..] == &rhs[..]
    }
}

impl Eq for Bytes {}

impl fmt::Debug for Bytes {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Bytes({:?})", &self[..])
    }
}

unsafe impl Send for Bytes {}

impl Bytes {
    pub const fn empty() -> Self {
        Self {
            buf: None,
            offset: 0,
            len: 0,
        }
    }

    pub fn from_obj(obj: BufEnum) -> Self {
        Self {
            offset: 0,
            len: obj[..].len(),
            buf: Some(obj),
        }
    }

    pub fn split(self, ix: usize) -> (Self, Self) {
        if ix == self.len() {
            return (self, Bytes::empty());
        }
        let buf = self.buf.expect("Can't split an empty buffer");
        assert!(ix < self.len);
        let prefix = Self {
            buf: Some(buf.clone()),
            offset: self.offset,
            len: ix,
        };
        let suffix = Self {
            buf: Some(buf),
            offset: self.offset + ix,
            len: self.len - ix,
        };
        (prefix, suffix)
    }

    pub fn take_buffer(self) -> Option<BufEnum> {
        let buf = self.buf?;
        if buf.refcount() == 1 {
            Some(buf)
        } else {
            None
        }
    }

    pub unsafe fn take_buffer_unsafe(self) -> (BufEnum, usize, usize) {
        (self.buf.unwrap(), self.offset, self.len)
    }
}

impl Deref for Bytes {
    type Target = [u8];

    fn deref(&self) -> &[u8] {
        match self.buf {
            None => &[],
            Some(ref buf) => &buf[self.offset..(self.offset + self.len)],
        }
    }
}

pub struct BytesMut {
    buf: Rc<[u8]>,
}

impl PartialEq for BytesMut {
    fn eq(&self, rhs: &Self) -> bool {
        &self[..] == &rhs[..]
    }
}

impl Eq for BytesMut {}

impl fmt::Debug for BytesMut {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "BytesMut({:?})", &self[..])
    }
}

impl From<&[u8]> for BytesMut {
    fn from(buf: &[u8]) -> Self {
        let mut b = Self::zeroed(buf.len());
        b[..].copy_from_slice(buf);
        b
    }
}

impl BytesMut {
    pub fn from_buf(buf: Rc<[u8]>) -> Self {
        assert_eq!(Rc::strong_count(&buf), 1);
        Self { buf }
    }

    pub fn zeroed(capacity: usize) -> Self {
        assert!(capacity > 0);
        Self {
            buf: unsafe { Rc::new_zeroed_slice(capacity).assume_init() },
        }
    }

    pub fn freeze(self) -> Bytes {
        Bytes {
            offset: 0,
            len: self.buf.len(),
            buf: Some(BufEnum::Rust(self.buf)),
        }
    }
}

impl Deref for BytesMut {
    type Target = [u8];

    fn deref(&self) -> &[u8] {
        &self.buf[..]
    }
}

impl DerefMut for BytesMut {
    fn deref_mut(&mut self) -> &mut [u8] {
        Rc::get_mut(&mut self.buf).unwrap()
    }
}
