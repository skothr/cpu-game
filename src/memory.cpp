#include "memory.hpp"
#include "logging.hpp"


cMemory::cMemory(int numBytes, int memSpeed)
  : bytes(numBytes), speed(memSpeed), mData(new unsigned char[numBytes])
{

}

cMemory::~cMemory()
{
  delete [] mData;
}


uint8_t cMemory::access(int addr)
{
  return (uint8_t)mData[addr];
}

void cMemory::set(int addr, uint8_t val)
{
  mData[addr] = val;
}

void cMemory::load(int addr, uint8_t *data, int numBytes)
{
  memcpy(&mData[addr], data, numBytes);
}
