#include <stdexcept>
#include <iostream>
#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity) : capacity_( capacity ),buffer(){}
void Writer::push( string data )
{
  // Your code here.
  //(void)data;
  for(auto &ch:data){
    if(buffer.size()>=capacity_){
      //set_error();
      break;
    }
    send++;
    buffer.push_back(ch);
  }
}

void Writer::close()
{
  // Your code here.
  is_eof=true;
}

void Writer::set_error()
{
  // Your code here.
  is_error=true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return {is_eof};
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return {capacity_-buffer.size()};
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return {send};
}

string_view Reader::peek() const
{
  // Your code here.
  if(buffer.empty()){
    return "";
  }
  return {std::string(&buffer.front(),1)};
}

bool Reader::is_finished() const
{
  // Your code here.
  return {is_eof&&buffer.empty()};
}

bool Reader::has_error() const
{
  // Your code here.
  return {is_error};
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  cout<<"fuck!!!!!!!!!"<<endl;
  while(len && !buffer.empty()){
    buffer.pop_front();
    len--;
    recv++;
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return {buffer.size()};
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return {recv};
}
