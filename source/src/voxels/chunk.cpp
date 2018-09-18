#include "chunk.hpp"
#include <iostream>

const Indexer<Chunk::sizeX, Chunk::sizeY, Chunk::sizeZ> Chunk::mIndexer;
const Point3i Chunk::size{sizeX, sizeY, sizeZ};

Chunk::Chunk(const Point3i &worldPos)
  : mWorldPos(worldPos)
{
  for(auto &side : gBlockSides)
    {
      mNeighbors.emplace(side, nullptr);
    }
}

bool Chunk::isEmpty() const
{
  //LOGD("CHUNK BLOCKS: %d", (int)mNumBlocks);
  return mNumBlocks == 0;
}


Chunk* Chunk::getNeighbor(blockSide_t side)
{
  return mNeighbors[side];
}
void Chunk::setNeighbor(blockSide_t side, Chunk *neighbor)
{
  mNeighbors[side] = neighbor;
}
void Chunk::unsetNeighbor(blockSide_t side)
{
  mNeighbors[side] = nullptr;
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

bool Chunk::setBlock(int bx, int by, int bz, block_t type)
{
  const int bi = mIndexer.index(bx, by, bz);
  block_t &b = mBlocks[bi];
  if((type != block_t::NONE) != (b != block_t::NONE))
    {
      if(isComplexBlock(b))
        {
          mComplex.erase(bi);
          mNumBlocks--;
        }
      b = type;
      if(b != block_t::NONE)
        { mNumBlocks++; }
      return true;
    }
  else
    { return false; }
}
bool Chunk::setBlock(const Point3i &bp, block_t type)
{
  return setBlock(bp[0], bp[1], bp[2], type);
}

bool Chunk::setComplex(int bx, int by, int bz, CompleteBlock block)
{
  const int bi = mIndexer.index(bx, by, bz);
  block_t &b = mBlocks[bi];
  if((b != block_t::NONE) != (block.type != block_t::NONE))
    {
      b = block.type;
      auto iter = mComplex.find(bi);
      if(iter != mComplex.end())
        {
          if(block.data)
            {
              iter->second = (ComplexBlock*)block.data;
            }
          else
            {
              mComplex.erase(bi);
              mNumBlocks--;
            }
        }
      else
        {
          if(block.data)
            {
              mNumBlocks++;
              mComplex.emplace(bi, (ComplexBlock*)block.data);
            }
          else
            { return false; }
        }
      
      if(block.data)
        {
          auto iterPX = mComplex.find(mIndexer.index(bx+1, by, bz));
          if(iterPX != mComplex.end() && iterPX->second)
            {
              iterPX->second->makeConnection(block);
              //((ComplexBlock*)block.data)->makeConnection(iterPX->second);
            }
          auto iterPY = mComplex.find(mIndexer.index(bx, by+1, bz));
          if(iterPY != mComplex.end() && iterPY->second)
            {
              iterPY->second->makeConnection(block);
              //((ComplexBlock*)block.data)->makeConnection(iterPY->second);
            }
          auto iterPZ = mComplex.find(mIndexer.index(bx, by, bz+1));
          if(iterPZ != mComplex.end() && iterPZ->second)
            {
              iterPZ->second->makeConnection(block);
              //((ComplexBlock*)block.data)->makeConnection(iterPZ->second);
            }
          auto iterNX = mComplex.find(mIndexer.index(bx-1, by, bz));
          if(iterNX != mComplex.end() && iterNX->second)
            {
              iterNX->second->makeConnection(block);
              //((ComplexBlock*)block.data)->makeConnection(iterNX->second);
            }
          auto iterNY = mComplex.find(mIndexer.index(bx, by-1, bz));
          if(iterNY != mComplex.end() && iterNY->second)
            {
              iterNY->second->makeConnection(block);
              //((ComplexBlock*)block.data)->makeConnection(iterNY->second);
            }
          auto iterNZ = mComplex.find(mIndexer.index(bx, by, bz-1));
          if(iterNZ != mComplex.end() && iterNZ->second)
            {
              iterNZ->second->makeConnection(block);
              //((ComplexBlock*)block.data)->makeConnection(iterNZ->second);
            }
        }
      return true;
    }
  else
    { return false; }
}
bool Chunk::setComplex(const Point3i &bp, CompleteBlock block)
{
  return setComplex(bp[0], bp[1], bp[2], block);
}

void Chunk::update()
{
  for(auto &iter : mComplex)
    {
      iter.second->update();
    }
}

void Chunk::reset()
{
  mNumBlocks = 0;
  for(auto &b : mBlocks)
    { b = block_t(); }
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
