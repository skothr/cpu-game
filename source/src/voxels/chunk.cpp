#include "chunk.hpp"
#include <iostream>

const Indexer<Chunk::sizeX, Chunk::sizeY, Chunk::sizeZ> Chunk::mIndexer;
const Point3i Chunk::size{sizeX, sizeY, sizeZ};

Chunk::Chunk(const Point3i &worldPos)
  : mWorldPos(worldPos)
{ }
/*
Chunk::Chunk(const Point3i &worldPos, const std::array<Block, Chunk::totalSize> &data)
  : Chunk(worldPos)
{
  for(int bi = 0; bi < totalSize; bi++)
    {
      mBlocks[bi] = data[bi];
      Block &b = mBlocks[bi];
      if(b.type != block_t::NONE)
        {
          mNumBlocks++;
          if(b.data)
            {
              mNumBytes += b.data->dataSize();
              mFluids.emplace(bi, reinterpret_cast<FluidData*>(b.data));
            }
        }
    }
  mDirty = true;
}
*/
bool Chunk::isEmpty() const
{
  //LOGD("CHUNK BLOCKS: %d", (int)mNumBlocks);
  return mNumBlocks == 0;
}

Block* Chunk::at(int bi)
{ return &mBlocks[bi]; }
Block* Chunk::at(int bx, int by, int bz)
{ return &mBlocks[mIndexer.index(bx, by, bz)]; }
Block* Chunk::at(const Point3i &bp)
{ return &mBlocks[mIndexer.index(bp)]; }
block_t Chunk::getType(int bx, int by, int bz) const
{ return mBlocks[mIndexer.index(bx, by, bz)].type; }
block_t Chunk::getType(const Point3i &bp) const
{ return mBlocks[mIndexer.index(bp[0], bp[1], bp[2])].type; }
std::array<Block, Chunk::totalSize>& Chunk::data()
{ return mBlocks; }
const std::array<Block, Chunk::totalSize>& Chunk::data() const
{ return mBlocks; }

BlockData* Chunk::getData(const Point3i &bp)
{
  int bi = mIndexer.index(bp);
  auto iter = mFluids.find(bi);
  if(iter != mFluids.end())
    { return iter->second; }
  else
    { return nullptr; }
}

/*
FluidData& Chunk::getFluid(int bx, int by, int bz)
{ return mFluids[mIndexer.index(bx, by, bz)].data; }
FluidData& Chunk::getFluid(const Point3i &bp)
{ return mFluids[mIndexer.index(bp)].data; }
*/

bool Chunk::setBlock(int bx, int by, int bz, block_t type, BlockData *data)
{
  const int bi = mIndexer.index(bx, by, bz);
  Block &b = mBlocks[bi];
  if((type != block_t::NONE) != (b.type != block_t::NONE))
    {
      auto fIter = mFluids.find(bi);
      if(fIter != mFluids.end())
        {
          if(data && type != block_t::NONE)
            { // copy data
              *fIter->second = *reinterpret_cast<FluidData*>(data->copy());
            }
          else
            { // remove data
              mNumBytes -= fIter->second->dataSize();
              delete fIter->second;
              mFluids.erase(bi);
            }
        }
      else if(data && type != block_t::NONE)
        { // add new data
          mNumBytes += data->dataSize();
          mFluids.emplace(bi, reinterpret_cast<FluidData*>(data->copy()));
        }
      
      b.type = type;
      if(b.type != block_t::NONE)
        { mNumBlocks++; }
      return true;
    }
  else
    { return false; }
}
bool Chunk::setBlock(const Point3i &bp, block_t type, BlockData *data)
{
  return setBlock(bp[0], bp[1], bp[2], type, data);
}
/*
bool Chunk::setFluid(int bx, int by, int bz, block_t type, const FluidData *data)
{
  const int bi = mIndexer.index(bx, by, bz);
  Block &b = mBlocks[bi];
  if((type != block_t::NONE) != (b.type != block_t::NONE))
    {
      b.type = type;
      if(!isFluidBlock(b))
        {
          mNumBlocks++;
          mNumBytes += FluidData::dataSize;
          b.data = new FluidData(*data);
          mActive.emplace(bi, b.data);
        }
      else
        {
          mActive[bi] = *data;
        }

      return true;
    }
  else
    { return false; }
}
bool Chunk::setFluid(const Point3i &bp, block_t type, const FluidData *data)
{
  return setFluid(bp[0], bp[1], bp[2], type, data);
}
*/

/*
void Chunk::setData(const std::array<Block, Chunk::totalSize> &data)
{
  reset();
  mBlocks = data;
  for(int bi = 0; bi < totalSize; bi++)
    {
      Block &b = mBlocks[bi];
      if(b.type != block_t::NONE)
        {
          mNumBlocks++;
          if(isFluidBlock(b.type))
            {
              mNumBytes += b.data->dataSize();
              mFluids.emplace(bi, reinterpret_cast<FluidData*>(b.data));
            }
        }
    }
  mDirty = true;
  }*/

void Chunk::reset()
{
  mNumBlocks = 0;
  mNumBytes = totalSize * Block::dataSize;
  for(auto &b : mBlocks)
    { b = Block(); }
  for(auto &f : mFluids)
    { delete f.second; }
  mFluids.clear();
}

std::unordered_map<int, FluidData*> Chunk::getFluids()
{
  return mFluids;
}

bool Chunk::step(bool evap)
{
  bool result = false;
  std::vector<int> gone;
  for(auto &f : mFluids)
    {
      if(evap)
        {
          result |= f.second->step();
        }
      if(f.second->gone())
        { gone.push_back(f.first); }
    }
  for(auto &f : gone)
    {
      result = true;
      mNumBlocks--;
      mBlocks[f].type = block_t::NONE;
      mNumBytes -= mFluids[f]->dataSize();
      mFluids.erase(f);
    }
  return result;
}

// TODO: Run length encoding
int Chunk::serialize(std::vector<uint8_t> &dataOut) const
{
  dataOut.resize(mNumBytes);
  int offset = 0;
  for(int bi = 0; bi < totalSize; bi++)
    {
      const Block &block = mBlocks[bi];
      offset += block.serialize(&dataOut[offset]);

      if(isFluidBlock(block.type))
        {
          auto iter = mFluids.find(bi);
          if(iter != mFluids.end())
            {
              offset += reinterpret_cast<FluidData*>(iter->second)->serialize(&dataOut[offset]);
            }
        }
    }
  return offset;
}
void Chunk::deserialize(const std::vector<uint8_t> &dataIn)
{
  reset();
  
  int offset = 0;
  for(int bi = 0; bi < totalSize; bi++)
    {
      Block &block = mBlocks[bi];
      block.deserialize(&dataIn[offset], Block::dataSize);
      if(block.type != block_t::NONE)
        { mNumBlocks++; }
      offset += Block::dataSize;
      
      if(isFluidBlock(block.type))
        {
          FluidData *data = new FluidData(&dataIn[offset]);
          mNumBytes += data->dataSize();
          offset += data->dataSize();
          mFluids.emplace(bi, data);
        }
    }
  mDirty = true;
}
