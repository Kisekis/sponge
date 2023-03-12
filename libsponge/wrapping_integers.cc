#include "wrapping_integers.hh"
#include <iostream>
// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint64_t r = 2;
    r = r<<31;
    uint32_t warped = (n%(r)+ isn.raw_value())%(r);
    return WrappingInt32{warped};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 sn, WrappingInt32 isn, uint64_t checkpoint) {
    uint64_t r = 1;
    r = r<<32;
    uint32_t diff = (r - isn.raw_value())%r + sn.raw_value();
    uint64_t ret = static_cast<uint64_t>(diff);
    if(ret > checkpoint) return ret;
    uint64_t upperbound = (1ul<<31) + checkpoint - ret;
    int k = upperbound/(1ul<<32);
    return ret + k*(1ul<<32);
}
