#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

//template <typename... Targs>
//void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _buf(capacity,'\0'), _bitmap(capacity,false), _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
   // DUMMY_CODE(data, index, eof);
   // 1、判断是不是满了，满了就返回
   // 2、判断是不是已经接收过了，如果接受过了并且结束信号，就将结束标志置为true,然后返回
   // 3、将数据加入待合并列表里
   // 4、将能够合并的数据进行合并
   // 5、将合并了的数据写入bytestream中
   // 6、如果没有待接收的数据并且有结束信号，则将—eof置为true
   // 7、如果待合并列表为空，并且——eof为true,则将结束标志位置为true
   // if oupput is full, return
   if (index >= _head_index + _output.remaining_capacity()) {
		return;			       
   }
   //if data has been merged before
   if (index + data.size() < _head_index) {
		 //if eof and output is empty
		if (eof)
			_eof = true;
		return;
	}
    if (index + data.size() <= _head_index + _output.remaining_capacity()) {
		if(eof) _eof = true;
	}
	for (size_t i = index; i < index + data.size() && i < _head_index + _output.remaining_capacity(); i++) {
		if (i >= _head_index && !_bitmap[i-_head_index]) {
			_buf[i-_head_index] = data[i-index];
			_bitmap[i-_head_index] = true;
		// count++;
			_unassembled_bytes++;
		}
	}
	string str = "";
	while(_bitmap.front()) {
		str += _buf.front();
		_buf.pop_front();
		_buf.push_back('\0');
		_bitmap.pop_front();
		_bitmap.push_back(false);
	}
	size_t len = str.size();
    if(len>0){
		_unassembled_bytes -= len;
		_head_index += len;
		_output.write(str);
	}

	if (_eof && empty()) {
			_output.end_input();
	}



}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }
