// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

use crate::{
    fail::Fail,
    file_table::{
        File,
        FileDescriptor,
        FileTable,
    },
    operations::{
        OperationResult,
        ResultFuture,
    },
    
    scheduler::SchedulerHandle,
    sync::{
        Bytes,
        UnboundedReceiver,
        UnboundedSender,
    },
};
use futures_intrusive::channel::shared::generic_channel;
use hashbrown::HashMap;
use std::{
    cell::RefCell,
    collections::VecDeque,
    future::Future,
    pin::Pin,
    rc::Rc,
    task::{
        Context,
        Poll,
        Waker,
    },
    net::UdpSocket,
};

pub struct UdpPeer {

}

struct Listener {
    buf: VecDeque<(Option<ipv4::Endpoint>, Bytes)>,
    waker: Option<Waker>,
}

#[derive(Debug)]
struct Socket {
    posix: std::net::UdpSocket,
}

type OutgoingReq = (Option<ipv4::Endpoint>, ipv4::Endpoint, Bytes);
type OutgoingSender = UnboundedSender<OutgoingReq>;
type OutgoingReceiver = UnboundedReceiver<OutgoingReq>;

struct Inner {
    file_table: FileTable,
    sockets: HashMap<FileDescriptor, Socket>,
    bound: HashMap<ipv4::Endpoint, Rc<RefCell<Listener>>>,

    #[allow(unused)]
    handle: SchedulerHandle,
}

impl UdpPeer {
    pub fn new(file_table: FileTable) -> Self {
        let future = Self::background(rt.clone(), arp.clone(), rx);
        let handle = rt.spawn(future);
        let inner = Inner {
            file_table,
            sockets: HashMap::new(),
            handle,
        };
        Self {
            inner: Rc::new(RefCell::new(inner)),
        }
    }

    pub fn accept(&self) -> Fail {
        Fail::Malformed {
            details: "Operation not supported",
        }
    }

    pub fn socket(&self) -> FileDescriptor {
        let mut inner = self.inner.borrow_mut();
        let fd = inner.file_table.alloc(File::UdpSocket);
        let socket = Socket {
            posix: None,
        };
        assert(inner.sockets.insert(fd, socket).is_none());
        fd
    }

    pub fn bind(&self, fd: FileDescriptor, addr: ipv4::Endpoint) -> Result<(), Fail> {
        let mut inner = self.inner.borrow_mut();
        match inner.sockets.get_mut(&fd) {
            Some(Socket { ref mut posix, .. }) if posix.is_none() => {
                let s = UdpSocket::bind(addr);
                let s = match s {
                    Ok(s) => *posix = s,
                    _ => {
                        return Err(Fail::Malformed {
                            details: "Could not bind to address",
                        })
                    },
                }
                        
            },
            _ => {
                return Err(Fail::Malformed {
                    details: "Invalid file descriptor on bind",
                })
            },
        }
        let listener = Listener {
            buf: VecDeque::new(),
            waker: None,
        };
        assert!(inner
            .bound
            .insert(addr, Rc::new(RefCell::new(listener)))
            .is_none());

        Ok(())
    }

    pub fn connect(&self, fd: FileDescriptor, addr: ipv4::Endpoint) -> Result<(), Fail> {
        let mut inner = self.inner.borrow_mut();
        match inner.sockets.get_mut(&fd) {
            Some(Socket { ref mut posix .. }) if !posix.is_none() => {
                let r = posix.connect(addr)?;
          },
            _ => Err(Fail::Malformed {
                details: "Please bind the socket before connecting",
            }),
        }
    }

    pub fn receive(&self, fd: FileDescriptor) -> Result<(), Fail> {
        match inner.sockets.get_mut(&fd) {
            Some(Socket { ref mut posix .. })  => {
                let mut buf = [0; 10];
                let (number_of_bytes, remote) = posix.recv_from(&mut buf)
                    .expect("Didn't receive data");
                let filled_buf = &mut buf[..number_of_bytes];
                let listener = inner.bound.get_mut(&local).ok_or_else(|| Fail::Malformed {
                    details: "Port not bound",
                })?;
                let mut l = listener.borrow_mut();
                l.buf.push_back((remote, filled_buf));`
                l.waker.take().map(|w| w.wake());
            },
            _ => Err(Fail::Malformed {
                details: "Cannot find socket",
            }),
        }
        Ok(())
    }

    pub fn push(&self, fd: FileDescriptor, buf: Bytes) -> Result<(), Fail> {
        let inner = self.inner.borrow();
        match inner.sockets.get(&fd) {
            Some(Socket { ref mut posix ..}) =>
                let number_of_bytes = posix.send(buf)?;
            _ => {
                return Err(Fail::Malformed {
                    details: "Invalid file descriptor on push",
                })
            },
        };
    }

    pub fn pushto(&self, fd: FileDescriptor, buf: Bytes, to: ipv4::Endpoint) -> Result<(), Fail> {
        let inner = self.inner.borrow();
        match inner.sockets.get(&fd) {
            Some(Socket { ref mut posix ..}) =>
                let number_of_bytes = posix.send_to(to, buf)?;
            _ => {
                return Err(Fail::Malformed {
                    details: "Invalid file descriptor on push",
                })
            },
        };
    }

    pub fn pop(&self, fd: FileDescriptor) -> PopFuture {
        let inner = self.inner.borrow();
        let listener = match inner.sockets.get(&fd) {
            Some(Socket {
                local: Some(local), ..
            }) => Ok(inner.bound.get(&local).unwrap().clone()),
            _ => Err(Fail::Malformed {
                details: "Invalid file descriptor",
            }),
        };
        PopFuture { listener, fd }
    }

    pub fn close(&self, fd: FileDescriptor) -> Result<(), Fail> {
        let mut inner = self.inner.borrow_mut();
        let socket = match inner.sockets.remove(&fd) {
            Some(s) => s,
            None => {
                return Err(Fail::Malformed {
                    details: "Invalid file descriptor",
                })
            },
        };
        if let Some(local) = socket.local {
            assert!(inner.bound.remove(&local).is_some());
        }
        inner.file_table.free(fd);
        Ok(())
    }
}

pub struct PopFuture {
    pub fd: FileDescriptor,
    listener: Result<Rc<RefCell<Listener>>, Fail>,
}

impl Future for PopFuture {
    type Output = Result<(Option<ipv4::Endpoint>, Bytes), Fail>;

    fn poll(self: Pin<&mut Self>, ctx: &mut Context) -> Poll<Self::Output> {
        let self_ = self.get_mut();
        match self_.listener {
            Err(ref e) => Poll::Ready(Err(e.clone())),
            Ok(ref l) => {
                let mut listener = l.borrow_mut();
                match listener.buf.pop_front() {
                    Some(r) => return Poll::Ready(Ok(r)),
                    None => (),
                }
                let waker = ctx.waker();
                listener.waker = Some(waker.clone());
                Poll::Pending
            },
        }
    }
}

pub enum UdpOperation {
    Accept(FileDescriptor, Fail),
    Connect(FileDescriptor, Result<(), Fail>),
    Push(FileDescriptor, Result<(), Fail>),
    Pop(ResultFuture<PopFuture>),
}

impl Future for UdpOperation {
    type Output = ();

    fn poll(self: Pin<&mut Self>, ctx: &mut Context) -> Poll<()> {
        match self.get_mut() {
            UdpOperation::Accept(..) | UdpOperation::Connect(..) | UdpOperation::Push(..) => {
                Poll::Ready(())
            },
            UdpOperation::Pop(ref mut f) => Future::poll(Pin::new(f), ctx),
        }
    }
}

impl UdpOperation {
    pub fn expect_result(self) -> (FileDescriptor, OperationResult) {
        match self {
            UdpOperation::Push(fd, Err(e))
            | UdpOperation::Connect(fd, Err(e))
            | UdpOperation::Accept(fd, e) => (fd, OperationResult::Failed(e)),
            UdpOperation::Connect(fd, Ok(())) => (fd, OperationResult::Connect),
            UdpOperation::Push(fd, Ok(())) => (fd, OperationResult::Push),

            UdpOperation::Pop(ResultFuture {
                future,
                done: Some(Ok((addr, bytes))),
            }) => (future.fd, OperationResult::Pop(addr, bytes)),
            UdpOperation::Pop(ResultFuture {
                future,
                done: Some(Err(e)),
            }) => (future.fd, OperationResult::Failed(e)),

            _ => panic!("Future not ready"),
        }
    }
}
