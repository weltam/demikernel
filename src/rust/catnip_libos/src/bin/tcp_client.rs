#![feature(maybe_uninit_uninit_array)]
#![feature(try_blocks)]

#[macro_use]
extern crate log;

use std::time::{Instant, Duration};
use must_let::must_let;
use std::io::Write;
use std::str::FromStr;
use anyhow::{
    format_err,
    Error,
};
use dpdk_rs::load_mlx5_driver;
use std::env;
use catnip::{
    libos::LibOS,
    logging,
    operations::OperationResult,
    protocols::{
        ip,
        ipv4::{Endpoint},
        ethernet2::MacAddress,
    },
    runtime::Runtime,
    tracing,
};
use hashbrown::HashMap;
use std::{
    convert::TryFrom,
    ffi::{
        CString,
    },
    fs::File,
    io::Read,
    net::Ipv4Addr,
};
use yaml_rust::{
    Yaml,
    YamlLoader,
};
use std::sync::atomic::{Ordering, AtomicBool};

static SHUTDOWN: AtomicBool = AtomicBool::new(false);

fn main() {
    load_mlx5_driver();
    ctrlc::set_handler(|| {
        SHUTDOWN.store(true, Ordering::SeqCst);
    }).unwrap();

    let r: Result<_, Error> = try {
        tracing::init(1_000_000);

        let config_path = env::args().nth(1).unwrap();
        let mut config_s = String::new();
        File::open(config_path)?.read_to_string(&mut config_s)?;
        let config = YamlLoader::load_from_str(&config_s)?;

        let config_obj = match &config[..] {
            &[ref c] => c,
            _ => Err(format_err!("Wrong number of config objects"))?,
        };

        let local_ipv4_addr: Ipv4Addr = config_obj["catnip"]["my_ipv4_addr"]
            .as_str()
            .ok_or_else(|| format_err!("Couldn't find my_ipv4_addr in config"))?
            .parse()?;
        if local_ipv4_addr.is_unspecified() || local_ipv4_addr.is_broadcast() {
            Err(format_err!("Invalid IPv4 address"))?;
        }

        let mut arp_table = HashMap::new();
        if let Some(arp_table_obj) = config_obj["catnip"]["arp_table"].as_hash() {
            for (k, v) in arp_table_obj {
                let key_str = k.as_str()
                    .ok_or_else(|| format_err!("Couldn't find ARP table key in config"))?;
                let key = MacAddress::parse_str(key_str)?;
                let value: Ipv4Addr = v.as_str()
                    .ok_or_else(|| format_err!("Couldn't find ARP table key in config"))?
                    .parse()?;
                arp_table.insert(key, value);
            }
            println!("Pre-populating ARP table: {:?}", arp_table);
        }

        let mut disable_arp = false;
        if let Some(arp_disabled) = config_obj["catnip"]["disable_arp"].as_bool() {
            disable_arp = arp_disabled;
            println!("ARP disabled: {:?}", disable_arp);
        }

        let eal_init_args = match config_obj["dpdk"]["eal_init"] {
            Yaml::Array(ref arr) => arr
                .iter()
                .map(|a| {
                    a.as_str()
                        .ok_or_else(|| format_err!("Non string argument"))
                        .and_then(|s| CString::new(s).map_err(|e| e.into()))
                })
                .collect::<Result<Vec<_>, Error>>()?,
            _ => Err(format_err!("Malformed YAML config"))?,
        };

        let runtime = catnip_libos::dpdk::initialize_dpdk(
            local_ipv4_addr,
            &eal_init_args,
            arp_table,
            disable_arp,
        )?;
        logging::initialize();

        let mut libos = LibOS::new(runtime)?;
        let buf_sz: usize = std::env::var("BUFFER_SIZE").unwrap().parse().unwrap();

        let connect_addr = &config_obj["client"]["connect_to"];
        let host_s = connect_addr["host"].as_str().expect("Invalid host");
        let host = Ipv4Addr::from_str(host_s).expect("Invalid host");
        let port_i = connect_addr["port"].as_i64().expect("Invalid port");
        let port = ip::Port::try_from(port_i as u16)?;
        let endpoint = Endpoint::new(host, port);


        let mss: usize = std::env::var("MSS").unwrap().parse().unwrap();
        let hdr_size = 54;

        let num_bufs = (buf_sz - 1) / mss + 1;
        assert_eq!(num_bufs, 1);
        let mut pktmbuf = libos.rt().alloc_mbuf();
        for i in hdr_size..(hdr_size + buf_sz) {
            pktmbuf[i] = 'a' as u8;
        }
        let buf = catnip::sync::Bytes::from_obj(catnip::sync::BufEnum::DPDK(pktmbuf));
        let (_, buf) = buf.split(hdr_size);
        let (buf, _) = buf.split(buf_sz);

        // let mut bufs = Vec::with_capacity(num_bufs);
        // for i in 0..num_bufs {
        //     let start = i * mss;
        //     let end = std::cmp::min(start + mss, buf_sz);
        //     let len = end - start;
        //     let mut pktbuf: catnip::sync::Mbuf = libos.rt().alloc_mbuf();
        //     for j in hdr_size..(hdr_size + len) {
        //         pktbuf[j] = 'a' as u8;
        //     }
        //     let buf = catnip::sync::Bytes::from_obj(catnip::sync::BufEnum::DPDK(pktbuf));
        //     let (_, buf) = buf.split(hdr_size);
        //     let (buf, _) = buf.split(len);
        //     println!("Buf {}, {} bytes", i, buf.len());
        //     bufs.push(buf);
        // }
        let num_clients: usize = std::env::var("NUM_CLIENTS").unwrap().parse().unwrap();
        let mut qtokens = Vec::with_capacity(num_clients);
        error!("Starting with {} clients", num_clients);
        
        for _ in 0..num_clients {
            let sockfd = libos.socket(libc::AF_INET, libc::SOCK_STREAM, 0)?;
            let qtoken = libos.connect(sockfd, endpoint);
            qtokens.push(qtoken);
        }
        let mut i = 1;
        let logging_interval = Duration::from_secs(1);
        let mut last_log = Instant::now();
        let mut starts = std::collections::HashMap::with_capacity(num_clients);
        let mut h = histogram::Histogram::configure().precision(4).build().unwrap();
        while !SHUTDOWN.load(Ordering::Relaxed) {
           if i % 2 == 0 {
               let now = Instant::now();
               if now - last_log > logging_interval {
                   last_log = now;
                   let _ = print_histogram(&h);
               }
           }
           i += 1;

           match libos.wait_any2(&mut qtokens, Duration::from_secs(1)) {
               None => continue,
               Some((fd, OperationResult::Connect)) => {
                   trace!("Connected {}", fd);
                   starts.insert(fd, Instant::now());
                   qtokens.push(libos.push2(fd, buf.clone()));
               },
               Some((fd, OperationResult::Pop(_, popped_buf))) => {
                   trace!("Popped {} bytes from {}", popped_buf.len(), fd);
                   let duration = starts.remove(&fd).unwrap().elapsed();
                   h.increment(duration.as_nanos() as u64).unwrap();
                   libos.rt().donate_buffer(popped_buf);
                   starts.insert(fd, Instant::now());
                   qtokens.push(libos.push2(fd, buf.clone()));
               },
               Some((fd, OperationResult::Push)) => {
                   trace!("Pushed on {}", fd);
                   qtokens.push(libos.pop(fd));
               },
               e => panic!("Unexpected {:?}", e),
           }
        }

        // let mut push_tokens = Vec::with_capacity(num_bufs);
        // 'shutdown: while !SHUTDOWN.load(Ordering::Relaxed) {
        //     assert!(push_tokens.is_empty());
        //     for b in &bufs {
        //         let qtoken = libos.push2(sockfd, b.clone());
        //         push_tokens.push(qtoken);
        //     }
        //     libos.wait_all_pushes(&mut push_tokens);
        //     trace!("Done pushing");

        //     let mut bytes_popped = 0;
        //     while bytes_popped < buf_sz {
        //         let qtoken = libos.pop(sockfd);
        //         let popped_buf = loop {
        //             match libos.wait4(qtoken, Duration::from_secs(1)) {
        //                 None => {
        //                     if SHUTDOWN.load(Ordering::Relaxed) {
        //                         break 'shutdown;
        //                     }
        //                 },
        //                 Some((_, OperationResult::Pop(_, popped_buf))) => break popped_buf,
        //                 e => panic!("Unexpected return: {:?}", e),
        //             }
        //         };
        //         bytes_popped += popped_buf.len();
        //         libos.rt().donate_buffer(popped_buf);
        //     }
        //     assert_eq!(bytes_popped, buf_sz);
        //     trace!("Done popping");
        // }

        let (events, num_overflow) = tracing::dump();
        println!("Dropped {} tracing events", num_overflow);
        let mut f = std::io::BufWriter::new(std::fs::File::create("tcp_client.log").unwrap());
        for (name, ts) in events {
            writeln!(f, "{} {}", name, ts).unwrap();
        }
        drop(f);

    };
    r.unwrap_or_else(|e| panic!("Initialization failure: {:?}", e));
}

fn print_histogram(h: &histogram::Histogram) -> Result<(), &'static str> {
    println!("p25  {:?}", Duration::from_nanos(h.percentile(0.25)?));
    println!("p50  {:?}", Duration::from_nanos(h.percentile(0.50)?));
    println!("p75  {:?}", Duration::from_nanos(h.percentile(0.75)?));
    println!("p95  {:?}", Duration::from_nanos(h.percentile(0.95)?));
    println!("p99  {:?}", Duration::from_nanos(h.percentile(0.99)?));
    println!("Avg: {:?}", Duration::from_nanos(h.mean()?));
    Ok(())
}
