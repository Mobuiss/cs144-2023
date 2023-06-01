#include "wrapping_integers.hh"
#include <iostream>
using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return Wrap32 { zero_point + (uint32_t)( n << 32 >> 32 ) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  uint64_t res = raw_value_ - zero_point.raw_value_;
  if ( checkpoint < res ) {
    return res;
  }
  uint64_t l = ( ( ( checkpoint - res ) >> 32 ) << 32 ) + res;
  uint64_t r = ( ( ( ( checkpoint - res ) >> 32 ) + 1 ) << 32 ) + res;
  uint64_t ans = checkpoint - l < r - checkpoint ? l : r;
  return { ans };
}
