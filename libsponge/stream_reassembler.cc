#include "stream_reassembler.hh"
#include <algorithm>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), _first_unacceptable(capacity), buffer() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    _first_unread = _output.bytes_read();
    _first_unacceptable = _first_unread + _capacity;
    push_data_into_buffer(data, index, eof);
    write_data();
    if (empty() && is_eof)
        _output.end_input();
}

void StreamReassembler::push_data_into_buffer(const string &data, const size_t index, const bool eof) {
    if(data.empty()) {
        if(!eof) {
            return;
        } else if (index >= _first_unassembled) {
            is_eof = true;
            return;
        }
    }
    if (index + data.size() - 1 < _first_unassembled)
        return;
    bool this_eof = eof;

    size_t next_unassembled = _first_unassembled >= index ? _first_unassembled - index : 0;
    for (; next_unassembled < data.size(); next_unassembled ++) {
        if (index + next_unassembled >= _first_unacceptable) {
            this_eof = false;
            break;
        }  
        buffer.insert(sdata{data[next_unassembled], index + next_unassembled});
    }

    is_eof = is_eof || this_eof;
}

size_t StreamReassembler::unassembled_bytes() const { return buffer.size(); }

void StreamReassembler::write_data() {
    string s;
    size_t next_write = _first_unassembled;
    auto it = buffer.begin();
    while(it!= buffer.end()) {
        if (it->index != next_write)
            break;
        s += it->c;
        next_write++;
        it = buffer.erase(it);
    }

    if(!s.empty()) {
        _first_unassembled = next_write;
        _output.write(s);
    }
}

bool StreamReassembler::empty() const { return buffer.empty(); }

// bool StreamReassembler::is_buffer_full() {

// }
