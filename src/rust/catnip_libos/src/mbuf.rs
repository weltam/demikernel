use dpdk_rs::{
    rte_mempool,
    rte_mbuf,
    rte_mbuf_refcnt_update,
    rte_pktmbuf_free,
    rte_mbuf_refcnt_read,
};
use catnip::sync::BufTrait;
use std::ops::{Deref, DerefMut};
use std::slice;
use std::ptr;

pub struct Mbuf {
    pkt: *mut rte_mbuf,
}

impl Mbuf {
    pub fn new(pkt: *mut rte_mbuf) -> Self {
        Self { pkt }
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

impl BufTrait for Mbuf {
    fn refcount(&self) -> usize {
        let cnt = unsafe { rte_mbuf_refcnt_read(self.pkt) };
        cnt as usize
    }

    fn buf_clone(&self) -> Box<dyn BufTrait> {
        Box::new(self.clone())
    }
}
