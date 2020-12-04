use crate::{
    collections::watched::WatchedValue,
    fail::Fail,
    protocols::tcp::SeqNumber,
    sync::Bytes,
};
use std::{
    cell::RefCell,
    collections::{VecDeque, BTreeMap},
    num::Wrapping,
    task::{
        Context,
        Poll,
        Waker,
    },
    time::{
        Duration,
        Instant,
    },
};

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ReceiverState {
    Open,
    ReceivedFin,
    AckdFin,
}

const MAX_OUT_OF_ORDER: usize = 16;

#[derive(Debug)]
pub struct Receiver {
    pub state: WatchedValue<ReceiverState>,

    //                     |-----------------recv_window-------------------|
    //                base_seq_no             ack_seq_no             recv_seq_no
    //                     v                       v                       v
    // ... ----------------|-----------------------|-----------------------| (unavailable)
    //         received           acknowledged           unacknowledged
    //
    pub base_seq_no: WatchedValue<SeqNumber>,
    pub recv_queue: RefCell<VecDeque<Bytes>>,
    pub ack_seq_no: WatchedValue<SeqNumber>,
    pub recv_seq_no: WatchedValue<SeqNumber>,

    pub ack_deadline: WatchedValue<Option<Instant>>,

    pub max_window_size: u32,
    pub window_scale: u8,

    waker: RefCell<Option<Waker>>,

    out_of_order: RefCell<BTreeMap<SeqNumber, Bytes>>,
}

impl Receiver {
    pub fn new(seq_no: SeqNumber, max_window_size: u32) -> Self {
        let window_scale = std::env::var("WINDOW_SCALE").unwrap().parse().unwrap();
        trace!("Initializing receiver with max window {}, scale {}", max_window_size, window_scale);
        Self {
            state: WatchedValue::new(ReceiverState::Open),
            base_seq_no: WatchedValue::new(seq_no),
            recv_queue: RefCell::new(VecDeque::with_capacity(2048)),
            ack_seq_no: WatchedValue::new(seq_no),
            recv_seq_no: WatchedValue::new(seq_no),
            ack_deadline: WatchedValue::new(None),
            max_window_size,
            waker: RefCell::new(None),
            out_of_order: RefCell::new(BTreeMap::new()),
            window_scale,
        }
    }

    pub fn window_size(&self) -> u32 {
        let Wrapping(bytes_outstanding) = self.recv_seq_no.get() - self.base_seq_no.get();
        self.max_window_size - bytes_outstanding
    }

    pub fn current_ack(&self) -> Option<SeqNumber> {
        let ack_seq_no = self.ack_seq_no.get();
        let recv_seq_no = self.recv_seq_no.get();
        if ack_seq_no < recv_seq_no {
            Some(recv_seq_no)
        } else {
            None
        }
    }

    pub fn ack_sent(&self, seq_no: SeqNumber) {
        assert_eq!(seq_no, self.recv_seq_no.get());
        self.ack_deadline.set(None);
        self.ack_seq_no.set(seq_no);
    }

    pub fn peek(&self) -> Result<Bytes, Fail> {
        if self.base_seq_no.get() == self.recv_seq_no.get() {
            if self.state.get() != ReceiverState::Open {
                return Err(Fail::ResourceNotFound {
                    details: "Receiver closed",
                });
            }
            return Err(Fail::ResourceExhausted {
                details: "No available data",
            });
        }

        let segment = self
            .recv_queue
            .borrow_mut()
            .front()
            .expect("recv_seq > base_seq without data in queue?")
            .clone();

        Ok(segment)
    }

    pub fn recv(&self) -> Result<Option<Bytes>, Fail> {
        if self.base_seq_no.get() == self.recv_seq_no.get() {
            if self.state.get() != ReceiverState::Open {
                return Err(Fail::ResourceNotFound {
                    details: "Receiver closed",
                });
            }
            return Ok(None);
        }

        let segment = self
            .recv_queue
            .borrow_mut()
            .pop_front()
            .expect("recv_seq > base_seq without data in queue?");
        self.base_seq_no
            .modify(|b| b + Wrapping(segment.len() as u32));

        Ok(Some(segment))
    }

    pub fn poll_recv(&self, ctx: &mut Context) -> Poll<Result<Bytes, Fail>> {
        if self.base_seq_no.get() == self.recv_seq_no.get() {
            if self.state.get() != ReceiverState::Open {
                return Poll::Ready(Err(Fail::ResourceNotFound {
                    details: "Receiver closed",
                }));
            }
            *self.waker.borrow_mut() = Some(ctx.waker().clone());
            return Poll::Pending;
        }

        let segment = self
            .recv_queue
            .borrow_mut()
            .pop_front()
            .expect("recv_seq > base_seq without data in queue?");

        trace!("recv {} bytes at {}", segment.len(), self.base_seq_no.get());
        self.base_seq_no
            .modify(|b| b + Wrapping(segment.len() as u32));


        Poll::Ready(Ok(segment))
    }

    pub fn receive_fin(&self) {
        // Even if we've already ACKd the FIN, we need to resend the ACK if we receive another FIN.
        self.state.set(ReceiverState::ReceivedFin);
    }

    pub fn receive_data<RT: crate::runtime::Runtime>(&self, seq_no: SeqNumber, buf: Bytes, now: Instant, cb: &super::ControlBlock<RT>) -> Result<(), Fail> {
        let _s = tracy_client::static_span!();
        if self.state.get() != ReceiverState::Open {
            return Err(Fail::ResourceNotFound {
                details: "Receiver closed",
            });
        }

        let recv_seq_no = self.recv_seq_no.get();
        if recv_seq_no != seq_no {
            if seq_no > recv_seq_no {
                let mut out_of_order = self.out_of_order.borrow_mut();
                if !out_of_order.contains_key(&seq_no) {
                    while out_of_order.len() > MAX_OUT_OF_ORDER {
                        let (&key, _) = out_of_order.iter().rev().next().unwrap();
                        out_of_order.remove(&key);
                    }
                }
                out_of_order.insert(seq_no, buf);
                return Err(Fail::Ignored {
                    details: "Out of order segment (reordered)",
                });
            } else {
                if let Some(buf) = buf.take_buffer() {
                    cb.rt.donate_buffer(buf);
                }
                return Err(Fail::Ignored {
                    details: "Out of order segment (duplicate)",
                });
            }
        }

        let unread_bytes = self
            .recv_queue
            .borrow()
            .iter()
            .map(|b| b.len())
            .sum::<usize>();
        if unread_bytes + buf.len() > self.max_window_size as usize {
            if let Some(buf) = buf.take_buffer() {
                cb.rt.donate_buffer(buf);
            }
            return Err(Fail::Ignored {
                details: "Full receive window",
            });
        }

        self.recv_seq_no.modify(|r| r + Wrapping(buf.len() as u32));
        self.recv_queue.borrow_mut().push_back(buf);
        self.waker.borrow_mut().take().map(|w| w.wake());

        // TODO: How do we handle when the other side is in PERSIST state here?
        if self.ack_deadline.get().is_none() {
            // TODO: Configure this value (and also maybe just have an RT pointer here.)
            self.ack_deadline
                .set(Some(now + Duration::from_millis(500)));
        }

        let new_recv_seq_no = self.recv_seq_no.get();
        let old_data = {
            let mut out_of_order = self.out_of_order.borrow_mut();
            out_of_order.remove(&new_recv_seq_no)
        };
        if let Some(old_data) = old_data {
            info!("Recovering out-of-order packet at {}", new_recv_seq_no);
            if let Err(e) = self.receive_data(new_recv_seq_no, old_data, now, cb) {
                info!("Failed to recover out-of-order packet: {:?}", e);
            }
        }

        Ok(())
    }
}
