#include "tcp_receiver.hh"
#include <iostream>
using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  //cout<<"message:"<<string(message.payload)<<' '<<inbound_stream.bytes_pushed()<<endl;
  //cout<<message.seqno.unwrap( zero_point, inbound_stream.bytes_pushed() )<<endl;
  if ( message.SYN ) {
    syn_received = true;
    zero_point = message.seqno;
  }
  //tot_size+=message.payload.size();
  //cout<<tot_size<<endl;
  if(!syn_received)return ;
  reassembler.insert( message.seqno.unwrap( zero_point, inbound_stream.bytes_pushed() )+message.SYN-1,
                      message.payload,
                      message.FIN,
                      inbound_stream );
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  TCPReceiverMessage tcprmsg;
  if ( syn_received )
    tcprmsg.ackno = help.wrap(
      inbound_stream.bytes_pushed() + 1 + inbound_stream.is_closed(), zero_point );
  uint64_t res=inbound_stream.available_capacity();
  tcprmsg.window_size = res>UINT16_MAX?UINT16_MAX:res;
  return { tcprmsg };
}
