#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, pleas` replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    //DUMMY_CODE(seg);
	//如果没收到syn,则返回
	//收到syn,设置isn,第一次 index = 0;
	//受到fin，则把eof置为ture
	//计算index,其中checkpoint为byte_stream中写了多少个数据，即byte_writen
	
	TCPHeader head = seg.header();
	bool eof = false;
	
	//not receive syn
	if(!_syn_rec && !head.syn)
		return;
	
	string data = seg.payload().copy();
	
	if(!_syn_rec && head.syn){
		_syn_rec = true;
		isn = head.seqno;
		
		if(head.fin)
			eof = true;
		_reassembler.push_substring(data,0,eof);
	}

	if(head.fin)
		eof = true;

	uint64_t checkpoint = stream_out().bytes_written();

	uint64_t abs_seqno = unwrap(head.seqno,isn,checkpoint);
	uint64_t stream_index = abs_seqno - 1;
	_reassembler.push_substring(data,stream_index,eof);
			

}

optional<WrappingInt32> TCPReceiver::ackno() const { 
	if(!_syn_rec)
		return nullopt;
	uint64_t abs_seqno = stream_out().bytes_written();
	
	//如果fin信号生效的话，则fin信号会占掉一个序列号
	if(_fin_rec && stream_out().input_ended())
		abs_seqno++;
	//ackno应该加1
	return optional<WrappingInt32>(wrap(abs_seqno+1,isn));
}

size_t TCPReceiver::window_size() const {
	return stream_out().remaining_capacity();
}


