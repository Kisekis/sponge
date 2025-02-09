#include "byte_stream.hh"
#include <algorithm>
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : m_capacity(capacity){ }

size_t ByteStream::write(const string &data) {
    size_t write_count = min(data.size(), remaining_capacity());
    for (size_t i = 0; i < write_count; i++) {
        ql.push_back(data[i]);
    }
    m_bytes_written += write_count;
    return write_count;
    // DUMMY_CODE(data);
    // return {};
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len)  { 
    size_t peek_count = min(len, ql.size());
    string ret;
    int count = 0;
    for(auto it = ql.begin() ;count < peek_count;it++) {
        ret += *it;
        count++;
    }
    // for (size_t i = 0; i < peek_count; i++) {
    //     ret += q.front();
    //     q.pop_front();
    // }
    // string rett(ret);
    // reverse(rett.begin(),rett.end());
    // for(auto x : rett) {
    //     q.push_front(x);
    // }
    return ret;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t pop_count = min(len, ql.size());
    m_bytes_read += pop_count;
    for (size_t i = 0; i < pop_count; i++) {
        ql.pop_front();
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) { 
    string ret = peek_output(len);
    
    pop_output(len);
    return ret;
}

void ByteStream::end_input() { is_end = true; }

bool ByteStream::input_ended() const { return is_end; } //ByteStream的输入终止了

size_t ByteStream::buffer_size() const { return ql.size(); }

bool ByteStream::buffer_empty() const { return ql.empty(); }

bool ByteStream::eof() const { return is_end && ql.empty(); }//ByteStream被读完了

size_t ByteStream::bytes_written() const { return m_bytes_written; }

size_t ByteStream::bytes_read() const { return m_bytes_read; }

size_t ByteStream::remaining_capacity() const { return m_capacity - ql.size(); }
