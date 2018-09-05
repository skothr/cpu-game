#include "chunk.hpp"
#include <iostream>

const Indexer<Chunk::sizeX, Chunk::sizeY, Chunk::sizeZ> Chunk::mIndexer;
const Point3i Chunk::size{sizeX, sizeY, sizeZ};

Chunk::Chunk(const Point3i &worldPos)
  : mWorldPos(worldPos)
{ }
/*
Chunk::Chunk(const Point3i &worldPos, const std::array<block_t, Chunk::totalSize> &data)
  : Chunk(worldPos)
{
  for(int bi = 0; bi < totalSize; bi++)
    {
      mBlocks[bi] = data[bi];
      block_t &b = mBlocks[bi];
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

block_t* Chunk::at(int bi)
{ return &mBlocks[bi]; }
block_t* Chunk::at(int bx, int by, int bz)
{ return &mBlocks[mIndexer.index(bx, by, bz)]; }
block_t* Chunk::at(const Point3i &bp)
{ return &mBlocks[mIndexer.index(bp)]; }
block_t Chunk::getType(int bx, int by, int bz) const
{ return mBlocks[mIndexer.index(bx, by, bz)]; }
block_t Chunk::getType(const Point3i &bp) const
{ return mBlocks[mIndexer.index(bp[0], bp[1], bp[2])]; }
std::array<block_t, Chunk::totalSize>& Chunk::data()
{ return mBlocks; }
const std::array<block_t, Chunk::totalSize>& Chunk::data() const
{ return mBlocks; }

// BlockData* Chunk::getData(const Point3i &bp)
// {
//   int bi = mIndexer.index(bp);
//   auto iter = mFluids.find(bi);
//   if(iter != mFluids.end())
//     { return iter->second; }
//   else
//     { return nullptr; }
// }

/*
FluidData& Chunk::getFluid(int bx, int by, int bz)
{ return mFluids[mIndexer.index(bx, by, bz)].data; }
FluidData& Chunk::getFluid(const Point3i &bp)
{ return mFluids[mIndexer.index(bp)].data; }
*/

bool Chunk::setBlock(int bx, int by, int bz, block_t type)//, BlockData *data)
{
  const int bi = mIndexer.index(bx, by, bz);
  block_t &b = mBlocks[bi];
  if((type != block_t::NONE) != (b != block_t::NONE))
    {
      /*
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
      */
      b = type;
      if(b != block_t::NONE)
        { mNumBlocks++; }
      return true;
    }
  else
    { return false; }
}
bool Chunk::setBlock(const Point3i &bp, block_t type)//, block_tData *data)
{
  return setBlock(bp[0], bp[1], bp[2], type);//, data);
}

/*
bool Chunk::setFluid(int bx, int by, int bz, block_t type, const FluidData *data)
{
  const int bi = mIndexer.index(bx, by, bz);
  block_t &b = mBlocks[bi];
  if((type != block_t::NONE) != (b.type != block_t::NONE))
    {
      b.type = type;
      if(!isFluidblock_t(b))
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
void Chunk::setData(const std::array<block_t, Chunk::totalSize> &data)
{
  reset();
  mBlocks = data;
  for(int bi = 0; bi < totalSize; bi++)
    {
      block_t &b = mBlocks[bi];
      if(b.type != block_t::NONE)
        {
          mNumBlocks++;
          if(isFluidblock_t(b.type))
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
  //mNumBytes = totalSize * block_t::dataSize;
  for(auto &b : mBlocks)
    { b = block_t(); }
  //for(auto &f : mFluids)
  //{ delete f.second; }
  //mFluids.clear();
}

// std::unordered_map<int, FluidData*> Chunk::getFluids()
// {
//   return mFluids;
// }

bool Chunk::step(bool evap)
{
  // bool result = false;
  // std::vector<int> gone;
  // for(auto &f : mFluids)
  //   {
  //     if(evap)
  //       {
  //         result |= f.second->step();
  //       }
  //     if(f.second->gone())
  //       { gone.push_back(f.first); }
  //   }
  // for(auto &f : gone)
  //   {
  //     result = true;
  //     mNumBlocks--;
  //     mBlocks[f].type = block_t::NONE;
  //     // mNumBytes -= mFluids[f]->dataSize();
  //     // mFluids.erase(f);
  //   }
  return false;
}

// TODO: Run length encoding
int Chunk::serialize(std::vector<uint8_t> &dataOut) const
{
  dataOut.resize(totalSize * sizeof(block_t));//mNumBytes);
  int offset = 0;
  for(int bi = 0; bi < totalSize; bi++)
    {
      std::memcpy((void*)&dataOut[offset], (void*)&mBlocks[bi], Block::dataSize);
      offset += Block::dataSize;
      
      /*
      if(isFluidBlock(block.type))
        {
          auto iter = mFluids.find(bi);
          if(iter != mFluids.end())
            {
              offset += reinterpret_cast<FluidData*>(iter->second)->serialize(&dataOut[offset]);
            }
        }
      */
    }
  return offset;
}
void Chunk::deserialize(const std::vector<uint8_t> &dataIn)
{
  reset();
  
  int offset = 0;
  for(int bi = 0; bi < totalSize; bi++)
    {
      std::memcpy((void*)&mBlocks[bi], (void*)&dataIn[offset], sizeof(block_t));
      if(mBlocks[bi] != block_t::NONE)
        { mNumBlocks++; }
      offset += Block::dataSize;

      /*
      if(isFluidBlock(block))
        {
          FluidData *data = new FluidData(&dataIn[offset]);
          mNumBytes += data->dataSize();
          offset += data->dataSize();
          mFluids.emplace(bi, data);
        }
      */
    }
  mDirty = true;
}
