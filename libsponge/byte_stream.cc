#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

using namespace std;

ByteStream::ByteStream(const size_t capacity): _capacity(capacity) {}

size_t ByteStream::write(const string &data) {
    if(remaining_capacity()==0)
			return 0;
	else{
		size_t len2write = min(remaining_capacity(),data.size());
		for(size_t i=0;i<len2write;i++)
			_buf.push_back(data[i]);
		_bytes_written += len2write;
		return len2write;
	}
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t len2peek = min(buffer_size(),len);
	return string().assign(_buf.begin(),_buf.begin()+len2peek);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
	size_t len2pop = min(buffer_size(),len);
	for(size_t i=0;i<len2pop;i++){
			_buf.pop_front();
	}
	_bytes_read += len2pop;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string ret = peek_output(len);
	pop_output(len);
	return ret;
}

void ByteStream::end_input() {
	_end_input = true;
}

bool ByteStream::input_ended() const { return _end_input; }

size_t ByteStream::buffer_size() const { return _buf.size(); }

bool ByteStream::buffer_empty() const { return buffer_size() == 0; }

bool ByteStream::eof() const { return _end_input && (buffer_size() == 0); }

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - _buf.size(); }
