#include "chunk.hpp"
#include <iostream>

const Indexer<Chunk::sizeX, Chunk::sizeY, Chunk::sizeZ> Chunk::mIndexer;
const Point3i Chunk::size{sizeX, sizeY, sizeZ};

Chunk::Chunk(const Point3i &worldPos)
  : mWorldPos(worldPos), mHash(Hash::hash(worldPos))
{
  for(auto &side : gBlockSides)
    {
      mNeighbors.emplace(side, nullptr);
      mNeighborHashes.emplace(side, Hash::hash(worldPos + sideDirection(side)));
    }
}

void Chunk::setWorldPos(const Point3i &pos)
{
  mWorldPos = pos;
  mHash = Hash::hash(pos);
  for(auto &side : gBlockSides)
    { mNeighborHashes[side] = Hash::hash(pos + sideDirection(side)); }
}
Point3i Chunk::pos() const
{ return mWorldPos; }
hash_t Chunk::hash() const
{ return mHash; }
hash_t Chunk::neighborHash(blockSide_t side)
{ return mNeighborHashes[side]; }


bool Chunk::isEmpty() const
{ return mNumBlocks == 0; }


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
      if(b != block_t::NONE)
        { mNumBlocks--; }
      else
        { mNumBlocks++; }
      if(isComplexBlock(b))
        { mComplex.erase(bi); }
      b = type;
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
      if(block.type == block_t::NONE)
        {
          mComplex.erase(bi);
          mNumBlocks--;
        }
      else if(block.data)
        {
          mComplex.emplace(bi, (ComplexBlock*)block.data);
          mNumBlocks++;
        }
      else // invalid data
        { return false; }
        
      b = block.type;
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
    { iter.second->update(); }
}


// Connected edges:
enum
  {
   NO_EDGE = 0x00,
   PX_NX = 0x01,
   PX_PY = 0x02,
   PX_NY = 0x04,
   PX_PZ = 0x08,
   PX_NZ = 0x10,
   NX_PY = 0x20,
   NX_NY = 0x40,
   NX_PZ = 0x80,
   NX_NZ = 0x100,
   PY_NY = 0x200,
   PY_PZ = 0x400,
   PY_NZ = 0x800,
   NY_PZ = 0x1000,
   NY_NZ = 0x2000,
   PZ_NZ = 0x4000,
   ALL_EDGES = 0x4FFF
  };


inline int edgeFlag(blockSide_t s1, blockSide_t s2)
{
  if((s1 == blockSide_t::PX && s2 == blockSide_t::NX) ||
     (s1 == blockSide_t::NX && s2 == blockSide_t::PX) )
    { return PX_NX; }
  else if((s1 == blockSide_t::PX && s2 == blockSide_t::PY) ||
          (s1 == blockSide_t::PY && s2 == blockSide_t::PX) )
    { return PX_PY; }
  else if((s1 == blockSide_t::PX && s2 == blockSide_t::NY) ||
          (s1 == blockSide_t::NY && s2 == blockSide_t::PX) )
    { return PX_NY; }
  else if((s1 == blockSide_t::PX && s2 == blockSide_t::PZ) ||
          (s1 == blockSide_t::PZ && s2 == blockSide_t::PX) )
    { return PX_PZ; }
  else if((s1 == blockSide_t::PX && s2 == blockSide_t::NZ) ||
          (s1 == blockSide_t::NZ && s2 == blockSide_t::PX) )
    { return PX_NZ; }
  else if((s1 == blockSide_t::NX && s2 == blockSide_t::PY) ||
          (s1 == blockSide_t::PY && s2 == blockSide_t::NX) )
    { return NX_PY; }
  else if((s1 == blockSide_t::NX && s2 == blockSide_t::NY) ||
          (s1 == blockSide_t::NY && s2 == blockSide_t::NX) )
    { return NX_NY; }
  else if((s1 == blockSide_t::NX && s2 == blockSide_t::PZ) ||
          (s1 == blockSide_t::PZ && s2 == blockSide_t::NX) )
    { return NX_PZ; }
  else if((s1 == blockSide_t::NX && s2 == blockSide_t::NZ) ||
          (s1 == blockSide_t::NZ && s2 == blockSide_t::NX) )
    { return NX_NZ; }
  else if((s1 == blockSide_t::PY && s2 == blockSide_t::NY) ||
          (s1 == blockSide_t::NY && s2 == blockSide_t::PY) )
    { return PY_NY; }
  else if((s1 == blockSide_t::PY && s2 == blockSide_t::PZ) ||
          (s1 == blockSide_t::PZ && s2 == blockSide_t::PY) )
    { return PY_PZ; }
  else if((s1 == blockSide_t::PY && s2 == blockSide_t::NZ) ||
          (s1 == blockSide_t::NZ && s2 == blockSide_t::PY) )
    { return PY_NZ; }
  else if((s1 == blockSide_t::NY && s2 == blockSide_t::PZ) ||
          (s1 == blockSide_t::PZ && s2 == blockSide_t::NY) )
    { return NY_PZ; }
  else if((s1 == blockSide_t::NY && s2 == blockSide_t::NZ) ||
          (s1 == blockSide_t::NZ && s2 == blockSide_t::NY) )
    { return NY_NZ; }
  else if((s1 == blockSide_t::PZ && s2 == blockSide_t::NZ) ||
          (s1 == blockSide_t::NZ && s2 == blockSide_t::PZ) )
    { return PZ_NZ; }
  else
    { return NO_EDGE; }
}

void Chunk::floodFill(std::queue<Point3i> &points,
                      std::array<bool, totalSize> &traversed,
                      blockSide_t &sides)//, blockSide_t enterSide)
{
  while(points.size() > 0)
    {
      Point3i bp = points.front();
      points.pop();
      
      const int bi = mIndexer.index(bp);
      if(!traversed[bi])
        {
          traversed[bi] = true;
          if(mBlocks[bi] == block_t::NONE)
            {
              if(bp[0] < sizeX-1)
                { points.push({bp[0]+1, bp[1],   bp[2]  }); }
              else
                { sides |= blockSide_t::PX; }
              if(bp[1] < sizeY-1)
                { points.push({bp[0],   bp[1]+1, bp[2]  }); }
              else
                { sides |= blockSide_t::PY; }
              if(bp[2] < sizeZ-1)
                { points.push({bp[0],   bp[1],   bp[2]+1}); }
              else
                { sides |= blockSide_t::PZ; }
              if(bp[0] > 0)
                { points.push({bp[0]-1, bp[1],   bp[2]  }); }
              else
                { sides |= blockSide_t::NX; }
              if(bp[1] > 0)
                { points.push({bp[0],   bp[1]-1, bp[2]  }); }
              else
                { sides |= blockSide_t::NY; }
              if(bp[2] > 0)
                { points.push({bp[0],   bp[1],   bp[2]-1}); }
              else
                { sides |= blockSide_t::NZ; }
            }
        }
    }
}

void Chunk::updateConnected()
{
  if(isEmpty())
    {
      mConnectedEdges = ALL_EDGES;
      return;
    }
  else
    { mConnectedEdges = NO_EDGE; }

  std::unordered_set<int> untraversed;
  std::unordered_set<int> edges;
  for(int bi = 0; bi < totalSize; bi++)
    {
      if(mBlocks[bi] == block_t::NONE)
        {
          untraversed.insert(bi);
          if(chunkEdge(mIndexer.unindex(bi)) != blockSide_t::NONE)
            { edges.insert(bi); }
        }
    }

  while(edges.size() > 0 && untraversed.size() > 0)
    {
      int bi = *edges.begin();
      edges.erase(bi);
      
      std::queue<int> blocks;
      blocks.push(bi);

      // flood fill empty space
      blockSide_t openSides = blockSide_t::NONE;
      while(blocks.size() > 0)
        {
          int bi = blocks.front();
          blocks.pop();
          
          if(untraversed.count(bi) > 0)
            { // block not traversed yet
              untraversed.erase(bi);
              Point3i p = mIndexer.unindex(bi);

              openSides |= (p[0] == sizeX-1 ? blockSide_t::PX : blockSide_t::NONE);
              openSides |= (p[1] == sizeY-1 ? blockSide_t::PY : blockSide_t::NONE);
              openSides |= (p[2] == sizeZ-1 ? blockSide_t::PZ : blockSide_t::NONE);
              openSides |= (p[0] == 0 ? blockSide_t::NX : blockSide_t::NONE);
              openSides |= (p[1] == 0 ? blockSide_t::NY : blockSide_t::NONE);
              openSides |= (p[2] == 0 ? blockSide_t::NZ : blockSide_t::NONE);
              if(p[0] > 0)
                { blocks.push(shiftNX(bi)); }
              if(p[0] < sizeX-1)
                { blocks.push(shiftPX(bi)); }
              if(p[1] > 0)
                { blocks.push(shiftPY(bi)); }
              if(p[1] < sizeY-1)
                { blocks.push(shiftNY(bi)); }
              if(p[2] > 0)
                { blocks.push(shiftPZ(bi)); }
              if(p[2] < sizeZ-1)
                { blocks.push(shiftNZ(bi)); }
            }
        }
      for(int i = 0; i < 6; i++)
        {
          for(int j = i+1; j < 6; j++)
            {
              int c = (1 << i) | (1 << j);
              if(((int)openSides & c) == c)
                { mConnectedEdges |= edgeFlag((blockSide_t)(1 << i), (blockSide_t)(1 << j)); }
            }
        }
    }
  
}

bool Chunk::sideOpen(blockSide_t side)
{
  switch(side)
    {
    case blockSide_t::PX:
      return (bool)(mConnectedEdges & (PX_NX|PX_PY|PX_NY|PX_PZ|PX_NZ));
    case blockSide_t::PY:
      return (bool)(mConnectedEdges & (PX_PY|NX_PY|PY_NY|PY_PZ|PY_NZ));
    case blockSide_t::PZ:
      return (bool)(mConnectedEdges & (PX_PZ|NX_PZ|PY_PZ|NY_PZ|PZ_NZ));
    case blockSide_t::NX:
      return (bool)(mConnectedEdges & (PX_NX|NX_PY|NX_NY|NX_PZ|NX_NZ));
    case blockSide_t::NY:
      return (bool)(mConnectedEdges & (PX_NY|NX_NY|PY_NY|NY_PZ|NY_NZ));
    case blockSide_t::NZ:
      return (bool)(mConnectedEdges & (PX_NZ|NX_NZ|PY_NZ|NY_NZ|PZ_NZ));
    default:
      return false;
    }
}

bool Chunk::edgesConnected(blockSide_t prevSide, blockSide_t nextSide)
{
  if(isEmpty())
    { return true; }
  else
    {
      const int edgeMask = edgeFlag(prevSide, nextSide);
      return (bool)((mConnectedEdges & edgeMask) == edgeMask);
    }
}
void Chunk::printEdgeConnections()
{
  LOGD("Connected edges: 0x%04X", (int)mConnectedEdges);
}

void Chunk::reset()
{
  mNumBlocks = 0;
  mComplex.clear();
  mConnectedEdges = 0;
  // for(auto &b : mBlocks)
  //   { b = block_t::NONE; }
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
  updateConnected();
}
