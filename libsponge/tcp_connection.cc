#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;
//接收方剩余容量
size_t TCPConnection::remaining_outbound_capacity() const { 
	return _sender.stream_in().remaining_capacity();;
}

size_t TCPConnection::bytes_in_flight() const { 
	return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const { 
	return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const { 
	return _time_since_last_segement_received;
}

void TCPConnection::segment_received(const TCPSegment &seg) { 
		//DUMMY_CODE(seg); 
	if(!_active)	return;
	
	_time_since_last_segement_received = 0;
	//RST标志，直接关闭
	if(seg.header().rst){
		_receiver.stream_out().set_error();
		_sender.stream_in().set_error();
		_active = false;
	}
	//当前是closed/listen状态
	else if(_sender.next_seqno_absolute() == 0){
		if(seg.header().syn){
			_receiver.segment_received(seg);
			connect();
		}
	}
	//当前是syn_sent状态
	else if(_sender.next_seqno_absolute() == _sender.bytes_in_flight() && !_receiver.ackno().has_value()){
		if (seg.header().syn && seg.header().ack) {
           // 收到SYN和ACK，说明由对方主动开启连接，进入Established状态，通过一个空包来发送ACK
           _sender.ack_received(seg.header().ackno, seg.header().win);
           _receiver.segment_received(seg);
           _sender.send_empty_segment();
           send_data();
       } else if (seg.header().syn && !seg.header().ack) {
           // 只收到了SYN，说明由双方同时开启连接，进入Syn-Rcvd状态，没有接收到对方的ACK，我们主动发一个
           _receiver.segment_received(seg);
           _sender.send_empty_segment();
           send_data();
       }	
	}
	//当前是syn_revd状态
	else if (_sender.next_seqno_absolute() == _sender.bytes_in_flight() && _receiver.ackno().has_value() && !_receiver.stream_out().input_ended()) {
       // 接收ACK，进入Established状态
       _sender.ack_received(seg.header().ackno, seg.header().win);
       _receiver.segment_received(seg);
   }
	//当前是established状态
	else if (_sender.next_seqno_absolute() > _sender.bytes_in_flight() && !_sender.stream_in().eof()) {
       // 发送数据，如果接到数据，则更新ACK
       _sender.ack_received(seg.header().ackno, seg.header().win);
       _receiver.segment_received(seg);
       if (seg.length_in_sequence_space() > 0) {
           _sender.send_empty_segment();
       }
       _sender.fill_window();
       send_data();
   }
	//当前是fin_wait_1状态
	else if (_sender.stream_in().eof() && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 && _sender.bytes_in_flight() > 0 && !_receiver.stream_out().input_ended()) {
       if (seg.header().fin) {
           // 收到Fin，则发送新ACK，进入Closing/Time-Wait
           _sender.ack_received(seg.header().ackno, seg.header().win);
           _receiver.segment_received(seg);
           _sender.send_empty_segment();
           send_data();
       } else if (seg.header().ack) {
           // 收到ACK，进入Fin-Wait-2
           _sender.ack_received(seg.header().ackno, seg.header().win);
           _receiver.segment_received(seg);
           send_data();
       }
   }
	//当前是fin_wait_2状态
	else if (_sender.stream_in().eof() && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 && _sender.bytes_in_flight() == 0 && !_receiver.stream_out().input_ended()) {
       _sender.ack_received(seg.header().ackno, seg.header().win);
       _receiver.segment_received(seg);
       _sender.send_empty_segment();
       send_data();
   }
	//当前是time_wait状态
	else if (_sender.stream_in().eof() && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 && _sender.bytes_in_flight() == 0 && _receiver.stream_out().input_ended()) {
       if (seg.header().fin) {
           // 收到FIN，保持Time-Wait状态
           _sender.ack_received(seg.header().ackno, seg.header().win);
           _receiver.segment_received(seg);
           _sender.send_empty_segment();
           send_data();
       }
   }
	//其他状态
	else {
       _sender.ack_received(seg.header().ackno, seg.header().win);
       _receiver.segment_received(seg);
       _sender.fill_window();
       send_data();
   }
}

bool TCPConnection::active() const { 
	return _active;
}

size_t TCPConnection::write(const string &data) {
    //DUMMY_CODE(data);
    if(data.empty())	return 0;

	//把数据放入sender中
	size_t size = _sender.stream_in().write(data);
	_sender.fill_window();
	send_data();
	return size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
		//DUMMY_CODE(ms_since_last_tick); 
	if(!_active)	return;
	_time_since_last_segement_received += ms_since_last_tick;

	_sender.tick(ms_since_last_tick);

	if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS){
		reset_connection();
		return;
	}
	//不知道干啥的
	send_data();
}

void TCPConnection::end_input_stream() {
	_sender.stream_in().end_input();
	_sender.fill_window();
	send_data();
}

void TCPConnection::connect() {
	_sender.fill_window();
	send_data();	
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
			
			reset_connection();
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}


void TCPConnection::reset_connection(){
	TCPSegment seg;
	seg.header().rst = true;
	_segments_out.push(seg);

	_receiver.stream_out().set_error();
	_sender.stream_in().set_error();
    _active = false;
}

void TCPConnection::send_data(){
	//将sender里的数据包装后通过connection发送
	while(!_sender.segments_out().empty()){
		TCPSegment seg = _sender.segments_out().front();
		_sender.segments_out().pop();
		if(_receiver.ackno().has_value()){
			seg.header().ack = true;
			seg.header().ackno = _receiver.ackno().value();
			seg.header().win = _receiver.window_size();
		}
		_segments_out.push(seg);
	}
	
	//如果发送完则结束发送
	if(_receiver.stream_out().input_ended()){
		if(!_sender.stream_in().eof()){
			_linger_after_streams_finish = false;
		}else if(_sender.bytes_in_flight() == 0){
			if(!_linger_after_streams_finish || _time_since_last_segement_received>= 10*_cfg.rt_timeout)
				_active = false;
		}
	}
}
