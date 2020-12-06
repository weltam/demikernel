#![feature(maybe_uninit_uninit_array)]
#![feature(try_blocks)]

use std::time::Duration;
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
        let listen_addr = &config_obj["server"]["bind"];
        let host_s = listen_addr["host"].as_str().expect("Invalid host");
        let host = Ipv4Addr::from_str(host_s).expect("Invalid host");
        let port_i = listen_addr["port"].as_i64().expect("Invalid port");
        let port = ip::Port::try_from(port_i as u16)?;
        let endpoint = Endpoint::new(host, port);

        let sockfd = libos.socket(libc::AF_INET, libc::SOCK_STREAM, 0)?;
        libos.bind(sockfd, endpoint)?;
        libos.listen(sockfd, 10)?;

        let mut qtokens = Vec::with_capacity(1024);
        qtokens.push(libos.accept(sockfd));
        let mut num_connections = 0;

        while !SHUTDOWN.load(Ordering::Relaxed) {
            match libos.wait_any2(&mut qtokens, Duration::from_secs(1)) {
                None => continue,
                Some((fd, OperationResult::Accept(new_fd))) => {
                    num_connections += 1;
                    println!("Accepted new connection, managing {} connections", num_connections);
                    assert_eq!(fd, sockfd);
                    qtokens.push(libos.accept(sockfd));
                    qtokens.push(libos.pop(new_fd));
                },
                Some((fd, OperationResult::Pop(_, buf))) => {
                    qtokens.push(libos.push2(fd, buf));
                    qtokens.push(libos.pop(fd));
                },
                Some((fd, OperationResult::Push)) => (),
                e => panic!("Unexpected operation result: {:?}", e),
            }
        }

        let (events, num_overflow) = tracing::dump();
        println!("Dropped {} tracing events", num_overflow);
        let filename = format!("events_tcp_server.log");
        let mut f = std::io::BufWriter::new(std::fs::File::create(&filename).unwrap());
        for (name, ts) in events {
            writeln!(f, "{} {}", name, ts).unwrap();
        }
        drop(f);

    };
    r.unwrap_or_else(|e| panic!("Initialization failure: {:?}", e));
}
