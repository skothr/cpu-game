#include "block.hpp"
#include "world.hpp"


/*
inline bool cBlock::dirty() const
{ return dirtySides != blockSide_t::NONE; }
inline bool cBlock::dirty(blockSize_t side) const
{ return (dirtySides & side) != blockSide_t::NONE; }
*/
bool cBlock::active() const
{ return type != block_t::NONE && activeSides != blockSide_t::NONE; }
bool cBlock::active(blockSide_t side) const
{ return type != block_t::NONE && (activeSides & side) != blockSide_t::NONE; }

bool cBlock::activeEmpty(blockSide_t side)
{
  activeSides |= side;
  return type != block_t::NONE;
}
bool cBlock::activeMirror(blockSide_t side)
{
  activeSides &= ~side;
  return type != block_t::NONE;
}

int cBlock::serialize(uint8_t *dataOut) const
{
  std::memcpy((void*)dataOut, (void*)&data, dataSize);
  return dataSize;
}
void cBlock::deserialize(const uint8_t *dataIn, int bytes)
{
  std::memcpy((void*)&data, (void*)dataIn, bytes);
  //activeSides = blockSide_t::ALL;
}
