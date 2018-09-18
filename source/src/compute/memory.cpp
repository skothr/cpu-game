#include "memory.hpp"
#include "logging.hpp"


Memory::Memory(int numBytes, int memSpeed)
  : bytes(numBytes), speed(memSpeed), mData(new unsigned char[numBytes])
{

}

Memory::~Memory()
{
  delete [] mData;
}


uint8_t Memory::access(int addr)
{
  return (uint8_t)mData[addr];
}

void Memory::set(int addr, uint8_t val)
{
  mData[addr] = val;
}

void Memory::load(int addr, uint8_t *data, int numBytes)
{
  memcpy(&mData[addr], data, numBytes);
}
