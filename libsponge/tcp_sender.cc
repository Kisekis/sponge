#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

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
    , outstanding_queue()
    , timer(_initial_retransmission_timeout)
    , _ackno(_isn) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    auto _size = _window_size == 0 ? 1 : _window_size;
    auto remaining_space = _size - (next_seqno() - _ackno);
    while(remaining_space > 0 && !finish) {
        // sending syn
        TCPSegment seg;
        if(_next_seqno == 0){
            seg.header().syn = true;
            seg.header().seqno = _isn;
            _next_seqno++;
            _bytes_in_flight++;
            _segments_out.push(seg);
            outstanding_queue.push(seg);
            remaining_space--;
            timer.start(cur_time);
            continue;
        }
        size_t payload_size = stream_in().buffer_size() < remaining_space ? stream_in().buffer_size() : remaining_space;
        payload_size = payload_size < TCPConfig::MAX_PAYLOAD_SIZE ? payload_size : TCPConfig::MAX_PAYLOAD_SIZE;
        if(payload_size != 0) { // payload 还有的发
            seg.payload() = stream_in().read(payload_size);
            seg.header().seqno = next_seqno();
            _bytes_in_flight += payload_size;
            _next_seqno += payload_size;
            remaining_space -= payload_size;
        } else if(payload_size == 0 && !stream_in().eof()) { //payload没得发了但是未中断，直接返回
            return;
        }
        if(stream_in().eof()) {
            if(remaining_space > 0) {
                if(payload_size == 0) {
                    seg.header().seqno = next_seqno();
                }
                seg.header().fin = true;
                _next_seqno++;
                _bytes_in_flight++;
                remaining_space--;
                finish = true;
            }
        }
        _segments_out.push(seg);
        if(!timer.is_start) {
            timer.start(cur_time);
        }
        outstanding_queue.push(seg);
       if(payload_size == 0) return; 
    }
    
    // fill the window with segments
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    this->_window_size = window_size;
    if(_ackno == _isn && ackno != _ackno + 1) {
        return;
    }
    if(unwrap(ackno,_isn,_next_seqno)<=unwrap(_ackno,_isn,_next_seqno))
        return;
    _bytes_in_flight -= (ackno - _ackno);
    _ackno = ackno;

    // pop outstanding segments
    while (!outstanding_queue.empty()) {
        auto front = outstanding_queue.front();
        auto last = front.header().seqno + front.length_in_sequence_space();
        if(ackno.raw_value() >= last.raw_value()) {
            outstanding_queue.pop();
        }else {
            break;
        }
    }
        // reset initial value
    timer.reset(_initial_retransmission_timeout);
    //
    if(!outstanding_queue.empty()) {
        timer.start(cur_time);
    }else if(finish){
        timer.close();
    }
    consecutive = 0; 
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    const size_t new_time = cur_time + ms_since_last_tick;
    if(timer.expired(new_time)) {
        _segments_out.push(outstanding_queue.front());
        if(_window_size) {
            consecutive++;
            timer.reset(); //double RTO
        }
        timer.start(new_time);
    }
    cur_time = new_time;
}

unsigned int TCPSender::consecutive_retransmissions() const { return consecutive; }

void TCPSender::send_empty_segment() { 
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
}
