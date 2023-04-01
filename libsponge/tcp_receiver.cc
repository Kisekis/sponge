#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if(stream_out().eof()) return;
    uint64_t stream_indices = 0;
    WrappingInt32 curr_seqno{0};
    //set the initial seqno if necessary
    if(!ISN_found && seg.header().syn) {
        this->ISN = seg.header().seqno;
        ISN_found = true;
        if(seg.payload().size() == 0) {
            if(seg.header().fin) {
                is_eof = true;
                _reassembler.push_substring("",0,true);
                return;
            }
            return;
        }else {
            all_data_size += seg.payload().size();
            goto push;
        }
    }else if(!ISN_found) {
        return;
    }
    all_data_size += seg.payload().size();
    curr_seqno = seg.header().seqno;
    stream_indices = unwrap(curr_seqno, ISN, all_data_size) - 1;
    //push any data, or end-of-stream marker, to the StreamReassembler
push:    
    string data = seg.payload().copy();
    bool eof = false;
    if(seg.header().fin) {
        eof = true;
        is_eof = true;
    }
    _reassembler.push_substring(data,stream_indices,eof);
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(!ISN_found) return optional<WrappingInt32>(nullopt);
    if(stream_out().input_ended()) return WrappingInt32{wrap(static_cast<uint64_t>(_reassembler._first_unassembled)+2, ISN)};
    return WrappingInt32{wrap(static_cast<uint64_t>(_reassembler._first_unassembled)+1, ISN)};
 }

size_t TCPReceiver::window_size() const { 
    return stream_out().remaining_capacity();
 }
