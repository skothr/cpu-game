#include "block.hpp"
#include "world.hpp"

#include "device.hpp"
#include "cpu.hpp"
#include "memory.hpp"


DeviceBlock::DeviceBlock()
  : mDevice(new Device())
{ }

DeviceBlock::~DeviceBlock()
{
  delete mDevice;
}

BlockData* DeviceBlock::copy() const
{
  return new DeviceBlock(*this);
}
void DeviceBlock::makeConnection(const CompleteBlock &other)
{
  LOGD("Device making connection!");
  if(other.type == block_t::MEMORY)
    {
      mDevice->addMemory(reinterpret_cast<MemoryBlock*>(other.data)->getMemory());
      LOGD("Connected to memory!");
    }
  else if(other.type == block_t::CPU)
    {
      mDevice->addCpu(reinterpret_cast<CpuBlock*>(other.data)->getCpu());
      LOGD("Connected to cpu!");
    }
}
// void DeviceBlock::removeConnection(const CompleteBlock &other)
// {
//   if(other.type == block_t::MEMORY)
//     {

//     }
//   else if(other.type == block_t::CPU)
//     {
      
//     } 
// }
void DeviceBlock::update()
{
  if(mDevice->ready())
    {
      mDevice->update();
    }
}

CpuBlock::CpuBlock(int numBits, int numRegs, int cpuSpeed)
  : mCpu(new Cpu(numBits, numRegs, cpuSpeed))
{ }
CpuBlock::~CpuBlock()
{
  delete mCpu;
}
BlockData* CpuBlock::copy() const
{
  return new CpuBlock(*this);
}
void CpuBlock::update()
{ }
void CpuBlock::makeConnection(const CompleteBlock &other)
{
  if(other.type == block_t::DEVICE)
    {
      LOGD("Memory making connection...");
      ((ComplexBlock*)other.data)->makeConnection({block_t::CPU, (BlockData*)this});
    }
}

MemoryBlock::MemoryBlock(int numBytes, int memSpeed)
  : mMemory(new Memory(numBytes, memSpeed))
{ }

MemoryBlock::~MemoryBlock()
{
  delete mMemory;
}
BlockData* MemoryBlock::copy() const
{
  return new MemoryBlock(*this);
}
void MemoryBlock::update()
{ }

void MemoryBlock::makeConnection(const CompleteBlock &other)
{
  if(other.type == block_t::DEVICE)
    {
      LOGD("Cpu making connection...");
      ((ComplexBlock*)other.data)->makeConnection({block_t::MEMORY, (BlockData*)this});
    }
}
