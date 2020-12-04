use std::time::Instant;

struct LoggingState {
    start: Instant,
    buf: Vec<(&'static str, u64)>,
    num_overflow: usize,
}

static mut LOGGER_STATE: Option<LoggingState> = None;

pub fn init(num_events: usize) {
    let st = LoggingState {
        start: Instant::now(),
        buf: Vec::with_capacity(num_events),
        num_overflow: 0,
    };
    unsafe {
        assert!(LOGGER_STATE.is_none());
        LOGGER_STATE = Some(st);
    }
}

#[inline]
pub fn log(name: &'static str) {
    let now = Instant::now();
    unsafe {
        let st = LOGGER_STATE.as_mut().unwrap();
        if st.buf.len() == st.buf.capacity() {
            st.num_overflow += 1;
            return;
        }
        let delta = now - st.start;
        let delta_ns = delta.as_nanos() as u64;
        st.buf.push((name, delta_ns));
    }
}

pub fn dump() -> (impl Iterator<Item=(&'static str, u64)>, usize) {
    unsafe {
        let st = LOGGER_STATE.take().unwrap();
        (st.buf.into_iter(), st.num_overflow)
    }
}
