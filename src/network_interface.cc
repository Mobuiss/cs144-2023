#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t next_ip=next_hop.ipv4_numeric();
  EthernetFrame ef;
  ef.header.src=ethernet_address_;
  cout<<time_now<<"ms"<<endl;
  if(arp_list.find(next_ip)!=arp_list.end()&&time_now-arp_list[next_ip].second<=update_time){
    auto data=serialize(dgram);cout<<"second="<<arp_list[next_ip].second<<"|"
    <<to_string(arp_list[next_ip].first)<<endl;
    ef.header.dst=arp_list[next_ip].first;
    ef.header.type=EthernetHeader::TYPE_IPv4;
    ef.payload=data;
    data_for_send.push(ef);
  }
  else {
    if(wait_for_send.find(next_ip)!=wait_for_send.end()){//need only one reply
      return ;
    }
    ef.header.dst=ETHERNET_BROADCAST;
    ef.header.type=EthernetHeader::TYPE_ARP;
    ARPMessage msg;
    msg.opcode=ARPMessage::OPCODE_REQUEST;
    msg.sender_ethernet_address=ethernet_address_;
    msg.sender_ip_address=ip_address_.ipv4_numeric();
    msg.target_ethernet_address={0,0,0,0,0,0};
    msg.target_ip_address=next_ip;
    ef.payload=serialize(msg);
    data_for_send.push(ef);
    wait_for_reply.push(Retransmit{time_now,ef,next_ip});
    wait_for_send[next_ip]=dgram;
  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  //parse : change payload to each data
  //serialize : change all data to a payload(include header and init payload)
  if(frame.header.type==EthernetHeader::TYPE_IPv4){
    InternetDatagram dgram;
    bool flag=parse(dgram,frame.payload);
    if(!flag)return nullopt;
    if(frame.header.dst==ethernet_address_){//ip datagram to me
      return dgram;
    }
  }
  else if(frame.header.type==EthernetHeader::TYPE_ARP){//ARP
    ARPMessage msg;
    bool flag=parse(msg,frame.payload);
    if(!flag)return nullopt;
    if(msg.target_ip_address!=ip_address_.ipv4_numeric())return nullopt;
    /*mapping ip to mac*/
    arp_list[msg.sender_ip_address]=pair<EthernetAddress,uint64_t>{msg.sender_ethernet_address,time_now};
    /*reply to sender*/
    if(msg.opcode==ARPMessage::OPCODE_REQUEST){
      msg.opcode=ARPMessage::OPCODE_REPLY;
      swap(msg.sender_ethernet_address,msg.target_ethernet_address);
      swap(msg.sender_ip_address,msg.target_ip_address);
      msg.sender_ethernet_address=ethernet_address_;
      EthernetFrame ef;
      ef.header.type=EthernetHeader::TYPE_ARP;
      ef.header.src=ethernet_address_;
      ef.header.dst=msg.target_ethernet_address;
      ef.payload=serialize(msg);
      data_for_send.push(ef);
    }
    else {//receive the reply and continue to send
      if(wait_for_send.find(msg.sender_ip_address)==wait_for_send.end())return nullopt;
      EthernetFrame ef;
      ef.header.src=ethernet_address_;
      ef.header.dst=msg.sender_ethernet_address;
      ef.header.type=EthernetHeader::TYPE_IPv4;
      ef.payload=serialize(wait_for_send[msg.sender_ip_address]);
      wait_for_send.erase(msg.sender_ip_address);
      data_for_send.push(ef);
    }
  }
  return nullopt;
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  time_now+=ms_since_last_tick;
  while(!wait_for_reply.empty()&&time_now-wait_for_reply.front().t>wait_time){
    auto data=wait_for_reply.front();
    if(arp_list.find(data.ip)!=arp_list.end()&&time_now-arp_list[data.ip].second<=update_time){
      wait_for_reply.pop();
      continue;
    }//puts("~~~~~~~~~~~~~~~~~~~~");
    data_for_send.push(data.ef);
    wait_for_reply.push(Retransmit{time_now,data.ef,data.ip});
    wait_for_reply.pop();
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if(data_for_send.empty())return nullopt;
  EthernetFrame ef=data_for_send.front();
  data_for_send.pop();
  return {ef};
}
