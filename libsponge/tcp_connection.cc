#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _curr_seg_time - _last_seg_time; }

void TCPConnection::segment_received(const TCPSegment &seg) { 
    if(seg.header().rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        // delete this;
        return;
    }else {
        _last_seg_time = _curr_seg_time;
        _receiver.segment_received(seg);
        if(seg.header().ack) {
            _sender.ack_received(seg.header().ackno, seg.header().win);
        }

        if(_receiver.ackno().has_value()) { //如果receiver ackno有值了，说明已经接受到了SYN
            send_segments();
            _sender.fill_window();
            
            if(_sender.segments_out().empty() && seg.length_in_sequence_space() > 0) {
                _sender.send_empty_segment();
            }
            send_segments();
            if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) { //inbound  在 outbound前结束了，
                _linger_after_streams_finish = false;
            }
        }
    }
 }

bool TCPConnection::active() const { 
    if (_sender.stream_in().error() || _receiver.stream_out().error()) { //error , active = false
        return false;
    }

    auto ended = _receiver.stream_out().input_ended();
    auto eof = _sender.stream_in().eof();
    auto eq2 = _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2;
    auto no_flight = _sender.bytes_in_flight() == 0;
    auto checked = ended && eof && eq2 && no_flight;

    // clean shut down
    if (!_linger_after_streams_finish) {   // inbound在outbound前结束了
        // # 1 ~ # 3 satisfied ->connection done
        if (checked) { //passive close
            return false; 
        }
        return true; //still alive
    }
    //_linger_after_streams_finish == true, need to linger
    if (checked) {
        if (time_since_last_segment_received() < 10 * _cfg.rt_timeout) {
            return true; //linger not enough, return true
        }    

        return false; //linger enough, checked enough, clean_shut_down, active = false
    }
    
    return true;// checked not satisfied, active = true

}

size_t TCPConnection::write(const string &data) {
    auto written_size = _sender.stream_in().write(data);
    _sender.fill_window();
    send_segments();
    return written_size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    _curr_seg_time += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    // send_segments();
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        // abort the connnection
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        TCPSegment seg;
        seg.header().seqno = _sender.next_seqno();
        seg.header().rst = true;
        while(!_sender.segments_out().empty()) {
            _sender.segments_out().pop();
        }
        _sender.segments_out().push(seg);
    } 
    // syn received
    else if (_receiver.ackno().has_value()) { 
        _sender.fill_window();
    }
    send_segments();
 }

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_segments();
}

void TCPConnection::connect() {
    // TCPSegment seg;
    // seg.header().seqno = _sender.next_seqno();
    // seg.header().syn = true;
    // _sender.segments_out().push(seg);
    _sender.fill_window();
    send_segments();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            TCPSegment seg;
            seg.header().seqno = _sender.next_seqno();
            seg.header().rst = true;
            _sender.segments_out().push(seg);
            send_segments();
            // Your code here: need to send a RST segment to the peer
        }   
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::send_segments() { 
    auto &sender_queue = _sender.segments_out();
    auto ackno = _receiver.ackno();
    auto window_size = _receiver.window_size();
    while (!sender_queue.empty()) {
        auto next = sender_queue.front();
        if(ackno.has_value()){
            next.header().ack = true;
            next.header().ackno = ackno.value();
        }
        next.header().win = static_cast<uint16_t>(min(window_size, static_cast<size_t>(numeric_limits<uint16_t>::max())));
        _segments_out.push(next);
        sender_queue.pop();
    }
}