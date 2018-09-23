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
  // mConnectedEdges.emplace(blockSide_t::PX, blockSide_t::NONE);
  // mConnectedEdges.emplace(blockSide_t::PY, blockSide_t::NONE);
  // mConnectedEdges.emplace(blockSide_t::PZ, blockSide_t::NONE);
  // mConnectedEdges.emplace(blockSide_t::NX, blockSide_t::NONE);
  // mConnectedEdges.emplace(blockSide_t::NY, blockSide_t::NONE);
  // mConnectedEdges.emplace(blockSide_t::NZ, blockSide_t::NONE);
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
   PZ_NZ = 0x4000
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
  mConnectedEdges = 0;

  int numTraversed = 0;
  static std::array<bool, totalSize> traversed;
  for(int bi = 0; bi < totalSize; bi++)
    { traversed[bi] = (mBlocks[bi] != block_t::NONE); }
    
  while(numTraversed < totalSize)
    {
      // find an untraversed block
      int bi;
      for(bi = 0; bi < totalSize; bi++)
        {
          if(!traversed[bi])
            { break; }
        }
      if(bi >= totalSize)
        { break; }
      
      std::queue<Point3i> blocks;
      std::unordered_set<blockSide_t> openSides;
      Point3i bp = mIndexer.unindex(bi);
      blocks.push(bp);

      // flood fill empty space
      while(blocks.size() > 0)
        {
          Point3i p = blocks.front();
          blocks.pop();
          
          int bi = mIndexer.index(p);
          if(!traversed[bi])
            { // block not traversed yet
              traversed[bi] = true;
              numTraversed++;
              if(p[0] > 0)
                { blocks.push({p[0]-1, p[1],   p[2]  }); }
              else
                { openSides.insert(blockSide_t::NX); }
              if(p[0] < sizeX-1)
                { blocks.push({p[0]+1, p[1],   p[2]  }); }
              else
                { openSides.insert(blockSide_t::PX); }
              
              if(p[1] > 0)
                { blocks.push({p[0],   p[1]-1, p[2]  }); }
              else
                { openSides.insert(blockSide_t::NY); }
              if(p[1] < sizeY-1)
                { blocks.push({p[0],   p[1]+1, p[2]  }); }
              else
                { openSides.insert(blockSide_t::PY); }
              
              if(p[2] > 0)
                { blocks.push({p[0],   p[1],   p[2]-1}); }
              else
                { openSides.insert(blockSide_t::NZ); }
              if(p[2] < sizeZ-1)
                { blocks.push({p[0],   p[1],   p[2]+1}); }
              else
                { openSides.insert(blockSide_t::PZ); }
            }
        }

      std::vector<blockSide_t> sideVec(openSides.begin(), openSides.end());
      for(int s = 0; s < sideVec.size(); s++)
        {
          for(int s2 = s+1; s2 < sideVec.size(); s2++)
            {
              mConnectedEdges |= edgeFlag(sideVec[s], sideVec[s2]);
            }
        }
      //LOGD("Connected edges: 0x%04X", (int)mConnectedEdges);
    }
  
  // std::queue<Point3i> points;
  
  // for(int bi = 0; bi < totalSize; bi++)
  //   {
  //     if(!traversed[bi] && mBlocks[bi] == block_t::NONE)
  //       { // empty unvisited block -- perform flood fill
  //         Point3i bp = mIndexer.unindex(bi);

  //         blockSide_t dir = blockSide_t::NONE;
          
  //         if(bp[0] == 0 || bp[0] == sizeX-1 ||
  //            bp[1] == 0 || bp[1] == sizeY-1 ||
  //            bp[2] == 0 || bp[2] == sizeZ-1 )
  //           { // block on edge of chunk
  //             blockSide_t connectedSides = blockSide_t::NONE;
  //             points.push(bp);
  //             floodFill(points, traversed, connectedSides);
              
  //             if(bp[0] == 0)
  //               { mConnectedEdges[blockSide_t::NX] |= connectedSides; }
  //             else if(bp[0] == sizeX-1)
  //               { mConnectedEdges[blockSide_t::PX] |= connectedSides; }
  //             if(bp[1] == 0)
  //               { mConnectedEdges[blockSide_t::NY] |= connectedSides; }
  //             else if(bp[1] == sizeY-1)
  //               { mConnectedEdges[blockSide_t::PY] |= connectedSides; }
  //             if(bp[2] == 0)
  //               { mConnectedEdges[blockSide_t::NZ] |= connectedSides; }
  //             else if(bp[2] == sizeZ-1)
  //               { mConnectedEdges[blockSide_t::PZ] |= connectedSides; }
  //           }
  //       }
  //   }
  // edges can't be connected to themselves
  //for(auto &iter : mConnectedEdges)
  //{ iter.second &= ~iter.first; }
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
  // LOGD("NX: %d", (int)mConnectedEdges[blockSide_t::NX]);
  // LOGD("PX: %d", (int)mConnectedEdges[blockSide_t::PX]);
  // LOGD("NY: %d", (int)mConnectedEdges[blockSide_t::NY]);
  // LOGD("PY: %d", (int)mConnectedEdges[blockSide_t::PY]);
  // LOGD("NZ: %d", (int)mConnectedEdges[blockSide_t::NZ]);
  // LOGD("PZ: %d", (int)mConnectedEdges[blockSide_t::PZ]);
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
