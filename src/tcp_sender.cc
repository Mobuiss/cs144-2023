#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <iostream>
#include <random>
using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) )
  , ackno( isn_ )
  , send_index( isn_ )
  , initial_RTO_ms_( initial_RTO_ms )
  , RTO( initial_RTO_ms )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return { have_read_bytes - have_ack_bytes };
}
uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return { retransmission_cnt };
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  // cout << "1111111111" << endl;
  if ( data_for_send.empty() )
    return nullopt;
  // cout << "2222222222" << endl;
  TCPSenderMessage tsm = data_for_send.front();
  data_for_send.pop();
  if ( tsm.sequence_length() == 0 )
    return tsm;
  uint64_t id = tsm.seqno.unwrap( isn_, checkpoint );
  if ( id != lowest_retransmission_index ) {
    if ( data_outstanding.empty() )
      lowest_retransmission_index = id;
    data_outstanding.push_back( tsm );
  }
  return { tsm };
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  // cout << "real_index=" << real_index << ' ' << "checkpoint=" << checkpoint << ' ' << "size=" << ' '
  //    << outbound_stream.bytes_buffered() << endl;

  while ( ( real_index < checkpoint + max( (uint16_t)1, window_size ) ) && !finish ) {
    string out;
    TCPSenderMessage tsm;
    uint64_t res = checkpoint + max( (uint16_t)1, window_size ) - real_index;
    if ( !has_syn ) {
      tsm.SYN = true;
      has_syn = true;
    }
    read( outbound_stream, min( res, TCPConfig::MAX_PAYLOAD_SIZE ) - tsm.SYN, out );
    tsm.payload = out;
    if ( res > tsm.sequence_length() && outbound_stream.is_finished() ) {
      tsm.FIN = true;
    }
    tsm.payload = out;
    tsm.seqno = send_index; // cout<<"send : "<<out<<endl;
    if ( tsm.sequence_length() == 0 )
      break;
    // cout<<"add"<<tsm.sequence_length()<<endl;
    have_read_bytes += tsm.sequence_length();
    send_index = send_index + tsm.sequence_length();
    real_index += tsm.sequence_length();
    data_for_send.push( tsm );
    if ( tsm.FIN )
      finish = true;
    if ( outbound_stream.is_finished() )
      break;
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.

  return TCPSenderMessage { Wrap32 { send_index }, false, ( string ) "", false };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  uint64_t res = checkpoint;
  if ( msg.ackno.has_value() ) {
    uint64_t x = msg.ackno.value().unwrap( isn_, checkpoint );
    if ( x > real_index || x < res )
      return;
    ackno = msg.ackno.value();
    checkpoint = x;
    have_ack_bytes = checkpoint;
  }
  window_size = msg.window_size;
  while ( !data_outstanding.empty()
          && data_outstanding.front().seqno.unwrap( isn_, checkpoint ) + data_outstanding.front().sequence_length()
               < checkpoint + 1 ) {
    // cout<<"popped!"<<endl;
    data_outstanding.pop_front();
  }
  if ( res < checkpoint ) {
    RTO = initial_RTO_ms_;
    retransmission_cnt = 0;
    if ( !data_outstanding.empty() ) {
      retransmission_timer = 0;
    }
  }
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  if ( !has_syn )
    return;
  retransmission_timer += ms_since_last_tick;
  if ( retransmission_timer < RTO )
    return;
  retransmission_timer = 0;
  if ( !data_outstanding.empty() ) {
    data_for_send.push( data_outstanding.front() );
  }
  if ( window_size ) {
    retransmission_cnt++;
    RTO <<= 1;
  }
}
