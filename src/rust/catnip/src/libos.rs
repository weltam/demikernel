use crate::{
    engine::{
        Engine,
        Protocol,
    },
    fail::Fail,
    file_table::FileDescriptor,
    interop::{
        dmtr_qresult_t,
        dmtr_sgarray_t,
    },
    protocols::ipv4::Endpoint,
    runtime::Runtime,
    operations::OperationResult,
    scheduler::{
        Operation,
        SchedulerHandle,
    },
    sync::{Bytes, BytesMut},
};
use libc::c_int;
use std::{
    slice,
    time::{Instant, Duration},
};

const TIMER_RESOLUTION: usize = 64;
const MAX_RECV_ITER: usize = 2;
pub type QToken = u64;

pub struct LibOS<RT: Runtime> {
    engine: Engine<RT>,
    rt: RT,

    ts_iters: usize,
}

impl<RT: Runtime> LibOS<RT> {
    pub fn new(rt: RT) -> Result<Self, Fail> {
        let engine = Engine::new(rt.clone())?;
        Ok(Self {
            engine,
            rt,
            ts_iters: 0,
        })
    }

    pub fn rt(&self) -> &RT {
        &self.rt
    }

    pub fn socket(
        &mut self,
        domain: c_int,
        socket_type: c_int,
        protocol: c_int,
    ) -> Result<FileDescriptor, Fail> {
        if domain != libc::AF_INET {
            return Err(Fail::Invalid {
                details: "Invalid domain",
            });
        }
        let engine_protocol = match socket_type {
            libc::SOCK_STREAM => Protocol::Tcp,
            libc::SOCK_DGRAM => Protocol::Udp,
            _ => {
                return Err(Fail::Invalid {
                    details: "Invalid socket type",
                })
            },
        };
        if protocol != 0 {
            return Err(Fail::Invalid {
                details: "Invalid protocol",
            });
        }
        Ok(self.engine.socket(engine_protocol))
    }

    pub fn bind(&mut self, fd: FileDescriptor, endpoint: Endpoint) -> Result<(), Fail> {
        self.engine.bind(fd, endpoint)
    }

    pub fn listen(&mut self, fd: FileDescriptor, backlog: usize) -> Result<(), Fail> {
        self.engine.listen(fd, backlog)
    }

    pub fn accept(&mut self, fd: FileDescriptor) -> QToken {
        let future = self.engine.accept(fd);
        self.rt.scheduler().insert(future).into_raw()
    }

    pub fn connect(&mut self, fd: FileDescriptor, remote: Endpoint) -> QToken {
        let future = self.engine.connect(fd, remote);
        self.rt.scheduler().insert(future).into_raw()
    }

    pub fn close(&mut self, fd: FileDescriptor) -> Result<(), Fail> {
        self.engine.close(fd)
    }

    pub fn push(&mut self, fd: FileDescriptor, sga: &dmtr_sgarray_t) -> QToken {
        // let _s = tracy_client::static_span!();
        let mut len = 0;
        for i in 0..sga.sga_numsegs as usize {
            len += sga.sga_segs[i].sgaseg_len;
        }
        let mut buf = BytesMut::zeroed(len as usize);
        let mut pos = 0;
        for i in 0..sga.sga_numsegs as usize {
            let seg = &sga.sga_segs[i];
            let seg_slice = unsafe {
                slice::from_raw_parts(seg.sgaseg_buf as *mut u8, seg.sgaseg_len as usize)
            };
            buf[pos..(pos + seg_slice.len())].copy_from_slice(seg_slice);
            pos += seg_slice.len();
        }
        let buf = buf.freeze();
        let future = self.engine.push(fd, buf);
        self.rt.scheduler().insert(future).into_raw()
    }

    pub fn push2(&mut self, fd: FileDescriptor, buf: Bytes) -> QToken {
        let future = self.engine.push(fd, buf);
        self.rt.scheduler().insert(future).into_raw()
    }

    pub fn pushto(&mut self, fd: FileDescriptor, sga: &dmtr_sgarray_t, to: Endpoint) -> QToken {
        // let _s = tracy_client::static_span!();
        let mut len = 0;
        for i in 0..sga.sga_numsegs as usize {
            len += sga.sga_segs[i].sgaseg_len;
        }
        let mut buf = BytesMut::zeroed(len as usize);
        let mut pos = 0;
        for i in 0..sga.sga_numsegs as usize {
            let seg = &sga.sga_segs[i];
            let seg_slice = unsafe {
                slice::from_raw_parts(seg.sgaseg_buf as *mut u8, seg.sgaseg_len as usize)
            };
            buf[pos..(pos + seg_slice.len())].copy_from_slice(seg_slice);
            pos += seg_slice.len();
        }
        let buf = buf.freeze();
        let future = self.engine.pushto(fd, buf, to);
        self.rt.scheduler().insert(future).into_raw()
    }

    pub fn drop_qtoken(&mut self, qt: QToken) {
        drop(self.rt.scheduler().from_raw_handle(qt).unwrap());
    }

    pub fn pop(&mut self, fd: FileDescriptor) -> QToken {
        // let _s = tracy_client::static_span!();
        let future = self.engine.pop(fd);
        self.rt.scheduler().insert(future).into_raw()
    }

    // If this returns a result, `qt` is no longer valid.
    pub fn poll(&mut self, qt: QToken) -> Option<dmtr_qresult_t> {
        let handle = self.rt.scheduler().from_raw_handle(qt).unwrap();
        if !handle.has_completed() {
            handle.into_raw();
            return None;
        }
        Some(self.take_operation(handle, qt))
    }

    pub fn wait(&mut self, qt: QToken) -> dmtr_qresult_t {
        let handle = self.rt.scheduler().from_raw_handle(qt).unwrap();
        loop {
            self.poll_bg_work();
            if handle.has_completed() {
                return self.take_operation(handle, qt);
            }
        }
    }

    pub fn wait2(&mut self, qt: QToken) -> (FileDescriptor, OperationResult) {
        let handle = self.rt.scheduler().from_raw_handle(qt).unwrap();
        loop {
            self.rt.scheduler().poll();
            if handle.has_completed() {
                return self.take_operation2(handle);
            }
            let mut num_received = 0;
            while let Some(pkt) = self.rt.receive() {
                num_received += 1;
                if let Err(e) = self.engine.receive(pkt) {
                    warn!("Dropped packet: {:?}", e);
                }
                if num_received > MAX_RECV_ITER {
                    break;
                }
            }
            if self.ts_iters == 0 {
                // let _t = tracy_client::static_span!("advance_clock");
                self.rt.advance_clock(Instant::now());
            }
            self.ts_iters = (self.ts_iters + 1) % TIMER_RESOLUTION;
        }
    }

    pub fn wait3(&mut self, qt: QToken) -> (FileDescriptor, OperationResult) {
        let handle = self.rt.scheduler().from_raw_handle(qt).unwrap();
        loop {
            // crate::tracing::log("scheduler:start");
            self.rt.scheduler().poll();
            // crate::tracing::log("scheduler:end");
            if handle.has_completed() {
                return self.take_operation2(handle);
            }
            let mut num_received = 0;
            loop {
                // crate::tracing::log("rt_receive:start");
                let pkt = self.rt.receive();
                // crate::tracing::log("rt_receive:end");
                let pkt = match pkt {
                    Some(p) => p,
                    None => break,
                };
                num_received += 1;
                // crate::tracing::log("engine_receive:start");
                if let Err(e) = self.engine.receive(pkt) {
                    warn!("Dropped packet: {:?}", e);
                }
                // crate::tracing::log("engine_receive:end");
                if num_received > MAX_RECV_ITER {
                    break;
                }
            }
            if self.ts_iters == 0 {
                // let _t = tracy_client::static_span!("advance_clock");
                // crate::tracing::log("advance_clock:start");
                self.rt.advance_clock(Instant::now());
                // crate::tracing::log("advance_clock:end");
            }
            self.ts_iters = (self.ts_iters + 1) % TIMER_RESOLUTION;
        }
    }

    pub fn wait4(&mut self, qt: QToken, timeout: Duration) -> Option<(FileDescriptor, OperationResult)> {
        let handle = self.rt.scheduler().from_raw_handle(qt).unwrap();
        let mut deadline = None;
        for i in 0.. {
            self.rt.scheduler().poll();
            if handle.has_completed() {
                return Some(self.take_operation2(handle));
            }
            let mut num_received = 0;
            while let Some(pkt) = self.rt.receive() {
                num_received += 1;
                if let Err(e) = self.engine.receive(pkt) {
                    warn!("Dropped packet: {:?}", e);
                }
                if num_received > MAX_RECV_ITER {
                    break;
                }
            }
            if self.ts_iters == 0 {
                // let _t = tracy_client::static_span!("advance_clock");
                self.rt.advance_clock(Instant::now());
            }
            self.ts_iters = (self.ts_iters + 1) % TIMER_RESOLUTION;

            if iter >= 25 && deadline.is_none() {
                deadline = Some(Instant::now() + timeout);
            }
            if iter % 25 == 0 {
                if let Some(d) = deadline {
                    if Instant::now() >= d {
                        return None
                    }
                }
            }
        }
    }

    pub fn wait_any2(&mut self, qts: &mut Vec<QToken>, timeout: Duration) -> Option<(FileDescriptor, OperationResult)> {
        let mut iter = 0u64;
        let mut deadline = None;
        let (i, result) = 'wait: loop {
            self.poll_bg_work();
            for (i, &qt) in qts.iter().enumerate() {
                let handle = self.rt.scheduler().from_raw_handle(qt).unwrap();
                if handle.has_completed() {
                    break 'wait (i, self.take_operation2(handle));
                }
                handle.into_raw();
            }

            iter += 1;

            if iter >= 25 && deadline.is_none() {
                deadline = Some(Instant::now() + timeout);
            }
            if iter % 25 == 0 {
                if let Some(d) = deadline {
                    if Instant::now() >= d {
                        return None
                    }
                }
            }
        };
        qts.swap_remove(i);
        Some(result)
    }

    pub fn wait_any(&mut self, qts: &[QToken]) -> (usize, dmtr_qresult_t) {
        // let _s = tracy_client::static_span!();
        loop {
            self.poll_bg_work();
            for (i, &qt) in qts.iter().enumerate() {
                let handle = self.rt.scheduler().from_raw_handle(qt).unwrap();
                if handle.has_completed() {
                    return (i, self.take_operation(handle, qt));
                }
                handle.into_raw();
            }
        }
    }

    pub fn wait_all_pushes(&mut self, qts: &mut Vec<QToken>) {
        self.poll_bg_work();
        for qt in qts.drain(..) {
            let handle = self.rt.scheduler().from_raw_handle(qt).unwrap();
            assert!(handle.has_completed());
            must_let::must_let!(let (_, OperationResult::Push) = self.take_operation2(handle));
        }
    }

    fn take_operation(&mut self, handle: SchedulerHandle, qt: QToken) -> dmtr_qresult_t {
        let (qd, r) = match self.rt.scheduler().take(handle) {
            Operation::Tcp(f) => f.expect_result(),
            Operation::Udp(f) => f.expect_result(),
            Operation::Background(..) => panic!("Polled background operation"),
        };
        dmtr_qresult_t::pack(r, qd, qt)
    }

    fn take_operation2(&mut self, handle: SchedulerHandle) -> (FileDescriptor, OperationResult) {
        match self.rt.scheduler().take(handle) {
            Operation::Tcp(f) => f.expect_result(),
            Operation::Udp(f) => f.expect_result(),
            Operation::Background(..) => panic!("Polled background operation"),
        }
    }

    fn poll_bg_work(&mut self) {
        // let _s = tracy_client::static_span!();
        self.rt.scheduler().poll();
        while let Some(pkt) = self.rt.receive() {
            if let Err(e) = self.engine.receive(pkt) {
                warn!("Dropped packet: {:?}", e);
            }
        }
        if self.ts_iters == 0 {
            // let _t = tracy_client::static_span!("advance_clock");
            self.rt.advance_clock(Instant::now());
        }
        self.ts_iters = (self.ts_iters + 1) % TIMER_RESOLUTION;
    }
}
