// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

use crate::{
    engine::Engine,
    scheduler::{
        Operation,
        Scheduler,
        SchedulerHandle,
    },
    sync::{
        Bytes,
        BytesMut,
    },
    timer::{
        Timer,
        TimerRc,
    },
};
use futures::FutureExt;
use rand::{
    distributions::{
        Distribution,
        Standard,
    },
    rngs::SmallRng,
    seq::SliceRandom,
    Rng,
    SeedableRng,
};
use std::{
    cell::RefCell,
    collections::VecDeque,
    future::Future,
    net::Ipv4Addr,
    rc::Rc,
    time::{
        Duration,
        Instant,
    },
};

pub const ALICE_IPV4: Ipv4Addr = Ipv4Addr::new(192, 168, 1, 1);
pub const BOB_IPV4: Ipv4Addr = Ipv4Addr::new(192, 168, 1, 2);

pub fn new_alice(now: Instant) -> Engine {
    Engine::new().unwrap()
}

pub fn new_bob(now: Instant) -> Engine<TestRuntime> {
    Engine::new().unwrap()
}
