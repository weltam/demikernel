use byteorder::{NativeEndian, ByteOrder};
use crate::{
    bindings::{
        ETH_SPEED_NUM_NONE,
        IPPROTO_UDP,
        rte_eth_ntuple_filter,
        RTE_ETH_FILTER_NTUPLE,
        RTE_PKTMBUF_HEADROOM,
        RTE_ETH_FLOW_NONFRAG_IPV4_UDP,
        RTE_ETH_INPUT_SET_L3_DST_IP4,
        RTE_ETH_INPUT_SET_L4_UDP_DST_PORT,
        RTE_ETH_INPUT_SET_SELECT,
        rte_eth_addr,
        rte_eth_dev_filter_ctrl,
        RTE_ETH_FILTER_FDIR,
        rte_eth_dev_filter_supported,
        rte_delay_us_block,
        rte_eal_init,
        rte_eth_conf,
        rte_eth_dev_configure,
        rte_eth_dev_count_avail,
        rte_eth_dev_flow_ctrl_get,
        rte_eth_dev_flow_ctrl_set,
        rte_eth_dev_info_get,
        rte_eth_dev_is_valid_port,
        rte_eth_dev_start,
        rte_eth_fc_mode_RTE_FC_NONE as RTE_FC_NONE,
        rte_eth_find_next_owned_by,
        rte_eth_link,
        rte_eth_link_get_nowait,
        rte_eth_macaddr_get,
        rte_eth_promiscuous_enable,
        rte_eth_rx_mq_mode_ETH_MQ_RX_RSS as ETH_MQ_RX_RSS,
        rte_eth_rx_mq_mode_ETH_MQ_RX_NONE as ETH_MQ_RX_NONE,
        rte_eth_rx_queue_setup,
        rte_eth_rxconf,
        rte_eth_tx_mq_mode_ETH_MQ_TX_NONE as ETH_MQ_TX_NONE,
        rte_eth_tx_queue_setup,
        rte_eth_txconf,
        rte_ether_addr,
        rte_mbuf,
        rte_mempool,
        rte_pktmbuf_pool_create,
        rte_socket_id,
        RTE_ETH_MAX_LEN,
        ETH_LINK_FULL_DUPLEX,
        ETH_LINK_UP,
        ETH_RSS_IP,
        RTE_ETHER_MAX_LEN,
        RTE_ETH_DEV_NO_OWNER,
        RTE_MAX_ETHPORTS,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        DEV_TX_OFFLOAD_MULTI_SEGS,
RTE_FDIR_MODE_PERFECT,
         RTE_FDIR_PBALLOC_64K,
         RTE_FDIR_NO_REPORT_STATUS,
    },
    runtime::DPDKRuntime,
};
use anyhow::{
    bail,
    format_err,
    Error,
};
use catnip::protocols::ethernet2::MacAddress;
use std::{
    ffi::CString,
    mem::MaybeUninit,
    net::Ipv4Addr,
    ptr,
    time::Duration,
};
use hashbrown::HashSet;

macro_rules! expect_zero {
    ($name:ident ( $($arg: expr),* $(,)* )) => {{
        let ret = $name($($arg),*);
        if ret == 0 {
            Ok(0)
        } else {
            Err(format_err!("{} failed with {:?}", stringify!($name), ret))
        }
    }};
}


const MAX_QUEUES_PER_PORT: usize = 16;
const BASE_ETH_UDP_PORT: u16 = 10000;
const NUM_RX_RING_ENTRIES: usize = 4096;
const NUM_MBUFS: usize = NUM_RX_RING_ENTRIES * 2 - 1;
const MTU: usize = 1024;
const MBUF_SIZE: usize = mem::size_of::<rte_mbuf>() + RTE_PKTMBUF_HEADROOM + MTU;
const NUM_TX_RING_DESC: usize = 128;

struct ErpcDPDK {
    used_qp_ids: HashSet<(usize, usize)>,
    port_initialized: [bool; RTE_MAX_ETHPORTS],
    mempool_arr: [[*mut rte_mempool; MAX_QUEUES_PER_PORT]; RTE_MAX_ETHPORTS],
}

#[derive(Debug)]
struct Resolve {
    ipv4_addr: u32,
    mac_addr: [u8; 6],
    bandwidth: usize,
}

impl ErpcDPDK {
    pub fn new() -> Self {
        Self {
            used_qp_ids: HashSet::new(),
            port_initialized: [false; RTE_MAX_ETHPORTS],
            mempool_arr: [[ptr::null_mut(); MAX_QUEUES_PER_PORT]; RTE_MAX_ETHPORTS],
        }
    }

    pub fn dpdk_transport(&mut self, phy_port: u8, numa_node: usize) {
        let mut qp_id = None;
        let mut rx_flow_udp_port = 0;

        for i in 0..MAX_QUEUES_PER_PORT {
            if !self.used_qp_ids.contains(&(phy_port, i)) {
                qp_id = Some(i);
                rx_flow_udp_port = self.udp_port_for_queue(phy_port, qp_id);
                break;
            }
        }
        let qp_id = qp_id.expect("No queues available");
        self.used_qp_ids.insert((phy_port, qp_id));

        // Initialize DPDK
        let rte_argv = &["-c", "1", "-n", "7", "--log-level", "0", "-m", "1024"];
        let rte_argv: Vec<_> = rte_argv.iter().map(CStr::new).collect();
        let ret = unsafe { rte_eal_init(rte_argv.len() rte_argv.as_ptr() as *mut _) };
        assert!(ret >= 0, "Failed to initialize DPDK");

        // Initialize the port
        if !self.port_initialized[phy_port] {
            self.port_initialized[phy_port] = true;
            self.setup_phy_port(phy_port);
        }

        let mempool = self.mempool_arr[phy_port][qp_id];
        let resolve = self.resolve_phy_port();

        todo!();
    }

    fn resolve_phy_port(&self, phy_port: usize) -> Resolve {
        let mut mac: rte_ether_addr = unsafe { MaybeUninit::zeroed().assume_init() };
        unsafe { rte_eth_macaddr_get(phy_port, &mut mac as *mut _) };

        let link: rte_eth_link = unsafe { MaybeUninit::zeroed().assume_init() };
        unsafe { rte_eth_link_get(phy_port as u8, &link as *mut _) };
        assert_eq!(link.link_status, ETH_LINK_UP);
        assert_ne!(link.link_speed, ETH_SPEED_NUM_NONE);
        assert!(link.link_speed >= 10000);
        let bandwidth = link.link_speed * 1000 * 1000 / 8;
        Resolve {
            ipv4_addr: self.get_port_ipv4_addr(phy_port),
            mac_addr: mac.addr_bytes,
            bandwidth,
        }
    }

    fn udp_port_for_queue(&self, phy_port: usize, qp_id: usize) -> u16 {
        BASE_ETH_UDP_PORT + (py_port as u16 * MAX_QUEUES_PER_PORT) + qp_id as u16;
    }

    fn setup_phy_port(&mut self, phy_port: usize) {
        let nb_ports = unsafe { rte_eth_dev_count_avail() };
        assert!(nb_ports > 0);
        assert!(phy_port < nb_ports);

        let dev_info = unsafe {
            let mut d = MaybeUninit::zeroed();
            rte_eth_dev_info_get(port_id, d.as_mut_ptr());
            d.assume_init()
        };

        // Create per-thread RX and TX queues
        let mut eth_conf: rte_eth_conf = unsafe { MaybeUninit::zeroed().assume_init() };
        eth_conf.rxmode.mq_mode = ETH_MQ_RX_NONE;
        eth_conf.rxmode.max_pkt_len = RTE_ETH_MAX_LEN;
        eth_conf.rxmode.offloads = 0;

        eth_conf.txmode.mq_mode = ETH_MQ_RX_NONE;
        eth_conf.txmode.offloads = DEV_TX_OFFLOAD_MULTI_SEGS;

        eth_conf.fdir_conf.mode = RTE_FDIR_MODE_PERFECT;
        eth_conf.fdir.pballoc = RTE_FDIR_PBALLOC_64K;
        eth_conf.fdir.status = RTE_FDIR_NO_REPORT_STATUS;
        eth_conf.fdir_conf.mask.dst_port_mask = 0xffff;
        eth_conf.fdir_conf.drop_queue = 0;

        let ret = unsafe { rte_eth_dev_configure(
            port_id,
            rx_rings,
            tx_rings,
            &port_conf as *const _,
        ) };
        assert_eq!(ret, 0);

        // Set the flow director fields.
        assert_eq!(unsafe { rte_eth_dev_filter_supported(phy_port, RTE_ETH_FILTER_FDIR) }, 0);
        let mut fi = unsafe { MaybeUninit::zeroed().assume_init() };
        fi.info.input_set_conf.flow_type = RTE_ETH_FLOW_NONFRAG_IPV4_UDP;
        fi.info.input_set_conf.inset_size = 2;
        fi.info.input_set_conf.field[0] = RTE_ETH_INPUT_SET_L3_DST_IP4;
        fi.info.input_set_conf.field[1] = RTE_ETH_INPUT_SET_L4_UDP_DST_PORT;
        fi.info.input_set_conf.op = RTE_ETH_INPUT_SET_SELECT;

        let ret = unsafe {
            rte_eth_dev_filter_ctrl(phy_port, RTE_ETH_FILTER_FDIR, RTE_ETH_FILTER_SET, &fi as *const _)
        };
        assert_eq!(ret, 0);

        for i in 0..MAX_QUEUES_PER_PORT {
            let name = CString::new(format!("demikernel-{}-{}", phy_port, i)).unwrap();
            let pool = unsafe {
                rte_pktmbuf_pool_create(
                    name.as_ptr(),
                    NUM_MBUFS as u32,
                    0, // cache,
                    0, // priv size,
                    MBUF_SIZE,
                    numa_node,
                );
            };
            assert!(!pool.is_null());
            self.mempool_arr[phy_port][i] = pool;

            let mut rx_conf: rte_eth_rxconf = unsafe { MaybeUninit::zeroed().assume_init() };
            rx_conf.rx_thresh.pthresh = 8;
            rx_conf.rx_thresh.hthresh = 0;
            rx_conf.rx_thresh.wthresh = 0;
            rx_conf.rx_free_thresh = 0;
            rx_conf.rx_drop_en = 0;

            let ret = unsafe {
                rte_eth_rx_queue_setup(
                    phy_port,
                    i,
                    NUM_RX_RING_ENTRIES,
                    numa_node,
                    &rx_conf as *const _,
                    self.mempool_arr[phy_port][i],
                )
            };
            assert_eq!(ret, 0);

            let mut tx_conf: rte_eth_txconf = unsafe { MaybeUninit::zeroed().assume_init() };
            tx_conf.tx_thresh.pthresh = 32;
            tx_conf.tx_thresh.hthresh = 0;
            tx_conf.tx_thresh.wthresh = 0;
            tx_conf.tx_free_thresh = 30;
            tx_conf.tx_rs_thresh = 0;
            tx_conf.offloads = OFFLOADS;

            let ret = unsafe {
                rte_eth_tx_queue_setup(
                    phy_port,
                    i,
                    NUM_TX_RING_DESC,
                    numa_node,
                    &tx_conf as *const _,
                )
            };
            assert_eq!(ret, 0);

            self.install_flow_rule(phy_port, i);
        }

        let ret = unsafe { rte_eth_dev_start(phy_port) };
        assert_eq!(ret, 0);
    }

    fn get_port_ipv4_addr(&self, phy_port: usize) -> u32 {
        let mut mac: rte_ether_addr = unsafe { MaybeUninit::zeroed().assert_init() };
        unsafe { rte_eth_macaddr_get(phy_port, &mut mac as *mut _) };
        NativeEndian::read_u32(&mac.addr_bytes[0..2])
    }

    fn install_flow_rule(&self, phy_port: usize, qp_id: usize) {
        let ipv4_addr = self.get_port_ipv4_addr(phy_port);
        let udp_port = self.udp_port_for_queue(phy_port, i);

        assert_eq!(unsafe { rte_eth_dev_filter_supported(phy_port, RTE_ETH_FILTER_NTUPLE) }, 0);

        let ntuple: rte_eth_ntuple_filter = unsafe { MaybeUninit::zeroed().assume_init() };
        ntuple.flags = RTE_5TUPLE_FLAGS;
        ntuple.dst_port = udp_port.to_be();
        ntuple.dst_port_mask = std::u16::MAX;
        ntuple.dst_ip = ipv4_addr.to_be();
        ntuple.dst_ip_mask = std::u32::MAX;
        ntuple.proto = IPPROTO_UDP;
        ntuple.proto_mask = std::u8::MAX;
        ntuple.priority = 1;
        ntuple.queue = qp_id;

        let ret = unsafe {
            rte_eth_dev_filter_ctrl(
                phy_port,
                RTE_ETH_FILTER_NTUPLE,
                RTE_ETH_FILTER_ADD,
                &ntuple as *const _,
            )
        };
        assert_eq!(ret, 0);
    }
}

pub fn initialize_dpdk(
    local_ipv4_addr: Ipv4Addr,
    eal_init_args: &[CString],
) -> Result<DPDKRuntime, Error> {
    let eal_init_refs = eal_init_args
        .iter()
        .map(|s| s.as_ptr() as *mut u8)
        .collect::<Vec<_>>();
    unsafe {
        rte_eal_init(eal_init_refs.len() as i32, eal_init_refs.as_ptr() as *mut _);
    }
    let nb_ports = unsafe { rte_eth_dev_count_avail() };
    if nb_ports == 0 {
        bail!("No ethernet ports available");
    }
    eprintln!(
        "DPDK reports that {} ports (interfaces) are available.",
        nb_ports
    );

    let name = CString::new("default_mbuf_pool").unwrap();
    let num_mbufs = 8191;
    let mbuf_cache_size = 250;
    let mbuf_pool = unsafe {
        rte_pktmbuf_pool_create(
            name.as_ptr(),
            (num_mbufs * nb_ports) as u32,
            mbuf_cache_size,
            0,
            RTE_MBUF_DEFAULT_BUF_SIZE as u16,
            rte_socket_id() as i32,
        )
    };
    if mbuf_pool.is_null() {
        Err(format_err!("rte_pktmbuf_pool_create failed"))?;
    }
    let mut port_id = 0;
    {
        let owner = RTE_ETH_DEV_NO_OWNER as u64;
        let mut p = unsafe { rte_eth_find_next_owned_by(0, owner) as u16 };

        while p < RTE_MAX_ETHPORTS as u16 {
            // TODO: This is pretty hax, we clearly only support one port.
            port_id = p;
            initialize_dpdk_port(p, mbuf_pool)?;
            p = unsafe { rte_eth_find_next_owned_by(p + 1, owner) as u16 };
        }
    }

    // TODO: Where is this function?
    // if unsafe { rte_lcore_count() } > 1 {
    //     eprintln!("WARNING: Too many lcores enabled. Only 1 used.");
    // }

    let local_link_addr = unsafe {
        let mut m: MaybeUninit<rte_ether_addr> = MaybeUninit::zeroed();
        // TODO: Why does bindgen say this function doesn't return an int?
        rte_eth_macaddr_get(port_id, m.as_mut_ptr());
        MacAddress::new(m.assume_init().addr_bytes)
    };
    if local_link_addr.is_nil() || !local_link_addr.is_unicast() {
        Err(format_err!("Invalid mac address"))?;
    }

    Ok(DPDKRuntime::new(
        local_link_addr,
        local_ipv4_addr,
        port_id,
        mbuf_pool,
    ))
}

fn initialize_dpdk_port(port_id: u16, mbuf_pool: *mut rte_mempool) -> Result<(), Error> {
    let rx_rings = 1;
    let tx_rings = 1;
    let rx_ring_size = 128;
    let tx_ring_size = 512;
    let nb_rxd = rx_ring_size;
    let nb_txd = tx_ring_size;

    let rx_pthresh = 0;
    let rx_hthresh = 0;
    let rx_wthresh = 0;

    let tx_pthresh = 0;
    let tx_hthresh = 0;
    let tx_wthresh = 0;

    let dev_info = unsafe {
        let mut d = MaybeUninit::zeroed();
        rte_eth_dev_info_get(port_id, d.as_mut_ptr());
        d.assume_init()
    };

    let mut port_conf: rte_eth_conf = unsafe { MaybeUninit::zeroed().assume_init() };
    port_conf.rxmode.max_rx_pkt_len = RTE_ETHER_MAX_LEN;
    port_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
    port_conf.rx_adv_conf.rss_conf.rss_hf = ETH_RSS_IP as u64 | dev_info.flow_type_rss_offloads;
    port_conf.txmode.mq_mode = ETH_MQ_TX_NONE;

    let mut rx_conf: rte_eth_rxconf = unsafe { MaybeUninit::zeroed().assume_init() };
    rx_conf.rx_thresh.pthresh = rx_pthresh;
    rx_conf.rx_thresh.hthresh = rx_hthresh;
    rx_conf.rx_thresh.wthresh = rx_wthresh;
    rx_conf.rx_free_thresh = 32;

    let mut tx_conf: rte_eth_txconf = unsafe { MaybeUninit::zeroed().assume_init() };
    tx_conf.tx_thresh.pthresh = tx_pthresh;
    tx_conf.tx_thresh.hthresh = tx_hthresh;
    tx_conf.tx_thresh.wthresh = tx_wthresh;
    tx_conf.tx_free_thresh = 32;

    unsafe {
        expect_zero!(rte_eth_dev_configure(
            port_id,
            rx_rings,
            tx_rings,
            &port_conf as *const _,
        ))?;
    }

    let socket_id = 0;

    unsafe {
        for i in 0..rx_rings {
            expect_zero!(rte_eth_rx_queue_setup(
                port_id,
                i,
                nb_rxd,
                socket_id,
                &rx_conf as *const _,
                mbuf_pool
            ))?;
        }
        for i in 0..tx_rings {
            expect_zero!(rte_eth_tx_queue_setup(
                port_id,
                i,
                nb_txd,
                socket_id,
                &tx_conf as *const _
            ))?;
        }
        expect_zero!(rte_eth_dev_start(port_id))?;
        rte_eth_promiscuous_enable(port_id);
    }

    let mut fc_conf = unsafe {
        let mut f = MaybeUninit::zeroed();
        expect_zero!(rte_eth_dev_flow_ctrl_get(port_id, f.as_mut_ptr()))?;
        f.assume_init()
    };
    fc_conf.mode = RTE_FC_NONE;
    unsafe { expect_zero!(rte_eth_dev_flow_ctrl_set(port_id, &mut fc_conf as *mut _))? };

    if unsafe { rte_eth_dev_is_valid_port(port_id) } == 0 {
        bail!("Invalid port");
    }

    let sleep_duration = Duration::from_millis(100);
    let mut retry_count = 90;

    loop {
        unsafe {
            let mut link: MaybeUninit<rte_eth_link> = MaybeUninit::zeroed();
            rte_eth_link_get_nowait(port_id, link.as_mut_ptr());
            let link = link.assume_init();
            if link.link_status() as u32 == ETH_LINK_UP {
                let duplex = if link.link_duplex() as u32 == ETH_LINK_FULL_DUPLEX {
                    "full"
                } else {
                    "half"
                };
                eprintln!(
                    "Port {} Link Up - speed {} Mbps - {} duplex",
                    port_id, link.link_speed, duplex
                );
                break;
            }
            rte_delay_us_block(sleep_duration.as_micros() as u32);
        }
        if retry_count == 0 {
            bail!("Link never came up");
        }
        retry_count -= 1;
    }

    Ok(())
}

// pub unsafe fn rte_pktmbuf_free(mut m: *mut rte_mbuf) {
//     let mut m_next = ptr::null_mut();

//     while !m.is_null() {
//         m_next = (*m).next;
//         rte_pktmbuf_free_seg(m);
//         m = m_next;
//     }
// }

// unsafe fn rte_pktmbuf_free_seg(mut m: *mut rte_mbuf) {
//     m = rte_pktmbuf_prefree_seg(m);
//     if !m.is_null() {
//         rte_mbuf_raw_free(m);
//     }
// }

// unsafe fn rte_pktmbuf_prefree_seg(m: *mut rte_mbuf) -> *mut rte_mbuf {
//     if rte_mbuf_refcnt_read(m) == 1 {
//         if !rte_mbuf_direct(m) {
//             rte_pktmbuf_detach(m);
//         }
//         if !(*m).next.is_null() {
//             (*m).next = ptr::null_mut();
//             (*m).nb_segs = 1;
//         }

//         return m;
//     } else if rte_mbuf_refcnt_update(m, -1) == 0 {
//         if !rte_mbuf_direct(m) {
//             rte_pktmbuf_detach(m);
//         }
//         if !(*m).next.is_null() {
//             (*m).next = ptr::null_mut();
//             (*m).nb_segs = 1;
//         }
// 		rte_mbuf_refcnt_set(m, 1);
//         return m;
//     }
//     ptr::null_mut()
// }

// unsafe fn rte_mbuf_refcnt_read(m: *mut rte_buf) -> u16 {
//     rte_atomic16_read((*m).refcnt_atomic) as u16
// }

// unsafe fn rte_mbuf_raw_free(m: *mut rte_mbuf) {
//     todo!();
// }
