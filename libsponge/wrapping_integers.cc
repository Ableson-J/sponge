#include "wrapping_integers.hh"

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
    //DUMMY_CODE(n, isn);
	
	return WrappingInt32(static_cast<uint32_t>(n) + isn.raw_value());
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
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    //DUMMY_CODE(n, isn, checkpoint);
    uint32_t offset = n.raw_value() - isn.raw_value();
	uint64_t mask = static_cast<uint64_t>(UINT32_MAX) << 32;
	uint64_t top32 = mask & checkpoint;
	uint64_t num1 = top32 - (1ul<<32) + offset;
	uint64_t abs1 = checkpoint - num1;
	uint64_t num2 = top32 + offset;
	uint64_t abs2 = (checkpoint>num2) ? checkpoint - num2 : num2 - checkpoint;
	uint64_t num3 = top32 + (1ul<<32) + offset;
	uint64_t abs3 = num3 - checkpoint;
	if(top32==0){
			return (abs2>abs3) ? num3 : num2;
	}
	if(abs1>abs2)
			return (abs2>abs3) ? num3 : num2;
	return (abs1>abs3) ? num3 : num1;
}
