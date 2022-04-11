#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
	, _retran_to(retx_timeout){}

uint64_t TCPSender::bytes_in_flight() const { return _data_in_flight; }

void TCPSender::fill_window() {
	if(_fin_sent) return;

	if(!_syn_sent){
		TCPSegment seg;
		seg.header().syn = true;
		send_tcp(seg);
		_syn_sent = true;
		return;
	}
	TCPSegment seg;
	uint64_t window_size = reci_window>0?reci_window:1;

	if(_stream.eof() && window_size + _ack_seqno > _next_seqno){
		seg.header().fin = true;
		send_tcp(seg);
		_fin_sent = true;
		return;
	}

	while(!_stream.buffer_empty() && window_size + _ack_seqno > _next_seqno){
		uint64_t len = min(static_cast<size_t>(window_size-(_next_seqno-_ack_seqno)), TCPConfig::MAX_PAYLOAD_SIZE);
		len = min(len,_stream.buffer_size());
		seg.payload() = _stream.read(len);

		if(_stream.eof() && seg.length_in_sequence_space() < window_size){
			seg.header().fin = true;
			_fin_sent = true;
		}
		send_tcp(seg);
	}
}

void TCPSender::send_tcp(TCPSegment& seg){
	seg.header().seqno = wrap(_next_seqno, _isn);
	_segments_out.push(seg);
	_segment_sent.push(seg);

	_next_seqno += seg.length_in_sequence_space();
	_data_in_flight += seg.length_in_sequence_space();

	if(!_retran_status){
		_retran_status = true;
		_retran_count = 0;
		//_retran_to = _initial_retransmission_timeout;
	}
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
		//DUMMY_CODE(ackno, window_size);
	uint64_t abs_ackno = unwrap(ackno,_isn,_next_seqno);
	if(abs_ackno > _next_seqno) return;
	//std::cout << "abs_ackno = "<< abs_ackno << "  _nex_seqno = "<< _next_seqno << " _ack_seqno = "<<_ack_seqno << endl;

	if(abs_ackno >= _ack_seqno){
		_ack_seqno = abs_ackno;
		reci_window = window_size;
	}
	
	bool pop = false;
	while(!_segment_sent.empty()){
		//std::cout<<"sent_size =" <<  _segment_sent.size() << " data flight = "<<_data_in_flight <<endl; 
		TCPSegment seg = _segment_sent.front();
		//std::cout << seg.payload().copy() << endl;
		uint64_t temp = unwrap(seg.header().seqno,_isn,_next_seqno);
		if(abs_ackno < temp + seg.length_in_sequence_space())
			return;

		_segment_sent.pop();
		_data_in_flight -= seg.length_in_sequence_space();
		pop = true;

		_retran_timeout = 0;
		_retran_count = 0;
		_retran_to = _initial_retransmission_timeout;
	if(pop)
		fill_window();	
	}
	_retran_status = !_segment_sent.empty();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
		//DUMMY_CODE(ms_since_last_tick);
	if(!_retran_status) return;
	_retran_timeout += ms_since_last_tick;
	if(_retran_timeout >= _retran_to){
		_segments_out.push(_segment_sent.front());
		_retran_timeout = 0;
		if(reci_window>0 || _segment_sent.front().header().syn){
			_retran_count++;
			_retran_to *= 2;
		}
	}
}

unsigned int TCPSender::consecutive_retransmissions() const {
	return _retran_count;
}

void TCPSender::send_empty_segment() {
	TCPSegment seg;
	seg.header().seqno = wrap(_next_seqno,_isn);
	_segments_out.push(seg);
}
