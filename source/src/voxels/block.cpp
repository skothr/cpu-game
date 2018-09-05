#include "block.hpp"
#include "world.hpp"

#include "fluid.hpp"

/*
void Block::updateOcclusion()
{
  occlusion = ((type == block_t::NONE) ? 0 : 1);
}
bool Block::active() const
{ return type != block_t::NONE && activeSides != blockSide_t::NONE; }
bool Block::active(blockSide_t side) const
{ return type != block_t::NONE && (activeSides & side) != blockSide_t::NONE; }

bool Block::activeEmpty(blockSide_t side)
{
  activeSides |= side;
  return type != block_t::NONE;
}
bool Block::activeMirror(blockSide_t side)
{
  activeSides &= ~side;
  return type != block_t::NONE;
}
*/
// int Block::serialize(uint8_t *dataOut) const
// {
//   dataOut[0] = (uint8_t)type;
//   int bytes = dataSize;
//   //std::memcpy((void*)dataOut, (void*)&type, dataSize);
//   // if(data)
//   //   {
//   //     if(isFluidBlock(type))
//   //       {
//   //         bytes += reinterpret_cast<FluidData*>(data)->serialize(dataOut+dataSize);
//   //       }
//   //   }
//   return bytes;
// }
// void Block::deserialize(const uint8_t *dataIn, int bytes)
// {
//   //std::memcpy((void*)&type, (void*)dataIn, dataSize);
//   type = (block_t)dataIn[0];
//   // if(data)
//   //   {
//   //     delete data;
//   //     data = nullptr;
//   //   }
//   // if(bytes > dataSize)
//   //   {
//   //     if(isFluidBlock(type))
//   //       { data = new FluidData(dataIn+dataSize, bytes-dataSize); }
//   //   }
// }
