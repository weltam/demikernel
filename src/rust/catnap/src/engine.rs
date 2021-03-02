// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

use crate::{
    fail::Fail,
    file_table::{
        File,
        FileDescriptor,
        FileTable,
    },
    operations::ResultFuture,
    scheduler::Operation,
    sync::Bytes,
};
use std::{
    future::Future,
    net::{Ipv4Addr, UdpSocket, TcpListener, TcpStream}
    time::Duration,
};
use tracy_client::static_span;

#[cfg(test)]
use crate::protocols::ethernet2::MacAddress;
#[cfg(test)]
use hashbrown::HashMap;

pub struct Engine {
    file_table: FileTable,
    udp: UdpPeer,
    tcp: TcpPeer,
}

pub enum Protocol {
    Tcp,
    Udp,
}

impl Engine {
    pub fn new() -> Result<Self, Fail> {
        let file_table = FileTable::new();
        let udp = UdpPeer::new();
        Ok(Engine {
            file_table,
        })
    }

    pub fn receive(&mut self, fd: FileDescriptor, bytes: Bytes) -> Result<(), Fail> {
        match self.file_table.get(fd) {
            Some(File::TcpSocket) => tcp.receive(fd, bytes),
            Some(File::UdpSocket) => udp.receive(fd, bytes),
        }
    }

    pub fn socket(&mut self, protocol: Protocol) -> FileDescriptor {
        match protocol {
            Protocol::Tcp => self.tcp.socket(),
            Protocol::Udp => self.udp.socket(),
        }
    }

    pub fn connect(
        &mut self,
        fd: FileDescriptor,
        remote_endpoint: Endpoint,
    ) -> Operation<RT> {
        match self.file_table.get(fd) {
            Some(File::TcpSocket) => Operation::from(self.tcp.connect(fd, remote_endpoint)),
            Some(File::UdpSocket) => {
                let udp_op = UdpOperation::Connect(fd, self.udp.connect(fd, remote_endpoint));
                Operation::Udp(udp_op)
            },
            _ => panic!("TODO: Invalid fd"),
        }
    }

    pub fn bind(&mut self, fd: FileDescriptor, endpoint: Endpoint) -> Result<(), Fail> {
        match self.file_table.get(fd) {
            Some(File::TcpSocket) => self.tcp.bind(fd, endpoint),
            Some(File::UdpSocket) => self.udp.bind(fd, endpoint),
            _ => panic!("TODO: Invalid fd"),
        }
    }

    pub fn accept(&mut self, fd: FileDescriptor) -> Operation<RT> {
        match self.file_table.get(fd) {
            Some(File::TcpSocket) => Operation::from(self.tcp.accept(fd)),
            Some(File::UdpSocket) => {
                let udp_op = UdpOperation::Accept(fd, self.udp.accept());
                Operation::Udp(udp_op)
            },
            _ => panic!("TODO: Invalid fd"),
        }
    }

    pub fn listen(&mut self, fd: FileDescriptor, backlog: usize) -> Result<(), Fail> {
        match self.file_table.get(fd) {
            Some(File::TcpSocket) => self.tcp.listen(fd, backlog),
            Some(File::UdpSocket) => Err(Fail::Malformed {
                details: "Operation not supported",
            }),
            _ => panic!("TODO: Invalid fd"),
        }
    }

    pub fn push(&mut self, fd: FileDescriptor, buf: Bytes) -> Operation<RT> {
        match self.file_table.get(fd) {
            Some(File::TcpSocket) => Operation::from(self.tcp.push(fd, buf)),
            Some(File::UdpSocket) => {
                let udp_op = UdpOperation::Push(fd, self.udp.push(fd, buf));
                Operation::Udp(udp_op)
            },
            _ => panic!("TODO: Invalid fd"),
        }
    }

    pub fn pushto(&mut self, fd: FileDescriptor, buf: Bytes, to: ipv4::Endpoint) -> Operation<RT> {
        match self.file_table.get(fd) {
            Some(File::UdpSocket) => {
                let udp_op = UdpOperation::Push(fd, self.udp.pushto(fd, buf, to));
                Operation::Udp(udp_op)
            },
            _ => panic!("TODO: Invalid fd"),
        }
    }

    pub fn udp_push(&mut self, fd: FileDescriptor, buf: Bytes) -> Result<(), Fail> {
        self.udp.push(fd, buf)
    }

    pub fn udp_pop(&mut self, fd: FileDescriptor) -> UdpPopFuture {
        self.udp.pop(fd)
    }

    pub fn pop(&mut self, fd: FileDescriptor) -> Operation<RT> {
        match self.file_table.get(fd) {
            Some(File::TcpSocket) => Operation::from(self.tcp.pop(fd)),
            Some(File::UdpSocket) => {
                let udp_op = UdpOperation::Pop(ResultFuture::new(self.udp.pop(fd)));
                Operation::Udp(udp_op)
            },
            _ => panic!("TODO: Invalid fd"),
        }
    }

    pub fn close(&mut self, fd: FileDescriptor) -> Result<(), Fail> {
        match self.file_table.get(fd) {
            Some(File::TcpSocket) => self.tcp.close(fd),
            Some(File::UdpSocket) => self.udp.close(fd),
            _ => panic!("TODO: Invalid fd"),
        }
    }

    pub fn tcp_socket(&mut self) -> FileDescriptor {
        self.tcp.socket()
    }

    pub fn tcp_connect(
        &mut self,
        socket_fd: FileDescriptor,
        remote_endpoint: ipv4::Endpoint,
    ) -> ConnectFuture<RT> {
        self.tcp.connect(socket_fd, remote_endpoint)
    }

    pub fn tcp_bind(
        &mut self,
        socket_fd: FileDescriptor,
        endpoint: ipv4::Endpoint,
    ) -> Result<(), Fail> {
        self.tcp.bind(socket_fd, endpoint)
    }

    pub fn tcp_accept(&mut self, handle: FileDescriptor) -> AcceptFuture<RT> {
        self.tcp.accept(handle)
    }

    pub fn tcp_push(&mut self, socket_fd: FileDescriptor, buf: Bytes) -> PushFuture<RT> {
        self.tcp.push(socket_fd, buf)
    }

    pub fn tcp_pop(&mut self, socket_fd: FileDescriptor) -> PopFuture<RT> {
        self.ipv4.tcp.pop(socket_fd)
    }

    pub fn tcp_close(&mut self, socket_fd: FileDescriptor) -> Result<(), Fail> {
        self.tcp.close(socket_fd)
    }

    pub fn tcp_listen(&mut self, socket_fd: FileDescriptor, backlog: usize) -> Result<(), Fail> {
        self.tcp.listen(socket_fd, backlog)
    }
}
