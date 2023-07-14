#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  table.push_back( { route_prefix, prefix_length, next_hop, interface_num } );
}

void Router::route()
{
  for ( auto& aif : interfaces_ ) {
    optional<InternetDatagram> op_dgram;
    while ( ( op_dgram = aif.maybe_receive() ).has_value() ) {
      InternetDatagram dgram = op_dgram.value();
      if ( dgram.header.ttl <= 1 )
        continue;
      dgram.header.ttl--;
      // important!!!!!!!!
      dgram.header.compute_checksum();
      uint32_t pos = table.size(); // match pos
      int len = -1;                // match length
      for ( size_t i = 0; i < table.size(); ++i ) {
        if ( table[i].prefix_length > len
             && ( table[i].prefix_length == 0
                  || ( dgram.header.dst >> ( 32 - table[i].prefix_length ) )
                       == ( table[i].route_prefix >> ( 32 - table[i].prefix_length ) ) ) ) {
          pos = i;
          len = table[i].prefix_length;
        }
      }
      if ( len == -1 )
        continue;
      interface( table[pos].interface_num )
        .send_datagram( dgram, table[pos].next_hop.value_or( Address::from_ipv4_numeric( dgram.header.dst ) ) );
    }
  }
}
