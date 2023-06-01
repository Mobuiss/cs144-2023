#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <map>
#include <queue>
class TCPSender
{
private:
  bool finish = false;
  bool has_syn = false;
  Wrap32 isn_;
  Wrap32 ackno;      // the most recent ackno from receiver
  Wrap32 send_index; // first squence number that hasn't sent(32-bit)
  uint16_t window_size = 1;
  uint64_t initial_RTO_ms_;
  uint64_t have_read_bytes = 0;      // the number of bytes read from Reader
  uint64_t have_ack_bytes = 0;       // the number of bytes that haven't ack yet
  uint64_t retransmission_cnt = 0;   // retranssmission times
  uint64_t retransmission_timer = 0; // the time length since last retranssmission
  uint64_t real_index = 0;           // first squence number that hasn't sent(64-bit)
  uint64_t checkpoint = 0;           // first unassembled squence number
  uint64_t RTO;                      // the time length for retranssmission
  uint64_t lowest_retransmission_index = -1;
  std::queue<TCPSenderMessage> data_for_send {};    // message that wait for sending
  std::deque<TCPSenderMessage> data_outstanding {}; // messages that haven't ack
  void read( Reader& reader, uint64_t len, std::string& out )
  {
    out.clear();

    while ( reader.bytes_buffered() and out.size() < len ) {
      auto view = reader.peek();

      if ( view.empty() ) {
        throw std::runtime_error( "Reader::peek() returned empty string_view" );
      }

      view = view.substr( 0, len - out.size() ); // Don't return more bytes than desired.
      out += view;
      reader.pop( view.size() );
    }
  }

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
