#![feature(maybe_uninit_uninit_array)]
#![feature(try_blocks)]

use std::time::Duration;
use histogram::Histogram;
use must_let::must_let;
use std::str::FromStr;
use catnip_libos::runtime::DPDKRuntime;
use anyhow::{
    format_err,
    Error,
};
use dpdk_rs::load_mlx5_driver;
use std::env;
use catnip::{
    sync::BytesMut,
    file_table::FileDescriptor,
    interop::{
        dmtr_qresult_t,
        dmtr_qtoken_t,
        dmtr_sgarray_t,
    },
    libos::LibOS,
    logging,
    operations::OperationResult,
    protocols::{
        ip,
        ipv4::{self, Endpoint},
        ethernet2::MacAddress,
    },
    runtime::Runtime,
};
use std::time::Instant;
use clap::{
    App,
    Arg,
};
use libc::{
    c_char,
    c_int,
    sockaddr,
    socklen_t,
};
use hashbrown::HashMap;
use std::{
    cell::RefCell,
    convert::TryFrom,
    ffi::{
        CStr,
        CString,
    },
    fs::File,
    io::Read,
    mem,
    net::Ipv4Addr,
    slice,
};
use yaml_rust::{
    Yaml,
    YamlLoader,
};

fn main() {
    load_mlx5_driver();

    let r: Result<_, Error> = try {
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

        if std::env::var("ECHO_SERVER").is_ok() {
            let listen_addr = &config_obj["server"]["bind"];
            let host_s = listen_addr["host"].as_str().expect("Invalid host");
            let host = Ipv4Addr::from_str(host_s).expect("Invalid host");
            let port_i = listen_addr["port"].as_i64().expect("Invalid port");
            let port = ip::Port::try_from(port_i as u16)?;
            let endpoint = Endpoint::new(host, port);

            let sockfd = libos.socket(libc::AF_INET, libc::SOCK_STREAM, 0)?;
            libos.bind(sockfd, endpoint)?;
            libos.listen(sockfd, 10)?;

            let qtoken = libos.accept(sockfd);
            must_let!(let (_, OperationResult::Accept(fd)) = libos.wait2(qtoken));
            println!("Accepted connection!");

            loop {
                let qtoken = libos.pop(fd);
                must_let!(let (_, OperationResult::Pop(_, buf)) = libos.wait2(qtoken));

                let qtoken = libos.push2(fd, buf);
                must_let!(let (_, OperationResult::Push) = libos.wait2(qtoken));
            }
        }
        else if std::env::var("ECHO_CLIENT").is_ok() {
            let num_iters: usize = std::env::var("NUM_ITERS").unwrap().parse().unwrap();
            let buf_sz: usize = std::env::var("BUFFER_SIZE").unwrap().parse().unwrap();

            let connect_addr = &config_obj["client"]["connect_to"];
            let host_s = connect_addr["host"].as_str().expect("Invalid host");
            let host = Ipv4Addr::from_str(host_s).expect("Invalid host");
            let port_i = connect_addr["port"].as_i64().expect("Invalid port");
            let port = ip::Port::try_from(port_i as u16)?;
            let endpoint = Endpoint::new(host, port);

            let sockfd = libos.socket(libc::AF_INET, libc::SOCK_STREAM, 0)?;
            let qtoken = libos.connect(sockfd, endpoint);
            must_let!(let (_, OperationResult::Connect) = libos.wait2(qtoken));

            let mut buf = BytesMut::zeroed(buf_sz);
            for b in &mut buf[..] {
                *b = 'a' as u8;
            }
            let buf = buf.freeze();

            let mut samples = Vec::with_capacity(num_iters);
            for _ in 0..num_iters {
                let start = Instant::now();
                let qtoken = libos.push2(sockfd, buf.clone());
                must_let!(let (_, OperationResult::Push) = libos.wait2(qtoken));

                let qtoken = libos.pop(sockfd);
                must_let!(let (_, OperationResult::Pop(..)) = libos.wait2(qtoken));
                samples.push(start.elapsed());
            }
            println!("Finished ({} samples)", num_iters);
            let mut h = Histogram::new();
            let mut total = 0;
            for s in samples {
                h.increment(s.as_nanos() as u64).unwrap();
                total += s.as_nanos();
            }

            println!("Min:   {:?}", Duration::from_nanos(h.minimum().unwrap()));
            println!(
                "p25:   {:?}",
                Duration::from_nanos(h.percentile(0.25).unwrap())
            );
            println!(
                "p50:   {:?}",
                Duration::from_nanos(h.percentile(0.50).unwrap())
            );
            println!("Avg:   {:?}", Duration::from_nanos((total as f64 / num_iters as f64) as u64));
            println!(
                "p75:   {:?}",
                Duration::from_nanos(h.percentile(0.75).unwrap())
            );
            println!(
                "p90:   {:?}",
                Duration::from_nanos(h.percentile(0.90).unwrap())
            );
            println!(
                "p95:   {:?}",
                Duration::from_nanos(h.percentile(0.95).unwrap())
            );
            println!(
                "p99:   {:?}",
                Duration::from_nanos(h.percentile(0.99).unwrap())
            );
            println!(
                "p99.9: {:?}",
                Duration::from_nanos(h.percentile(0.999).unwrap())
            );
            println!("Max:   {:?}", Duration::from_nanos(h.maximum().unwrap()));

        }
        else {
            panic!("Set either ECHO_SERVER or ECHO_CLIENT");
        }
    };
    r.unwrap_or_else(|e| panic!("Initialization failure: {:?}", e));
}
