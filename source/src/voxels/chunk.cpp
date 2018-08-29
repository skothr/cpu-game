#include "chunk.hpp"
#include <iostream>

const Indexer<Chunk::sizeX, Chunk::sizeY, Chunk::sizeZ> Chunk::mIndexer;
const Point3i Chunk::size{sizeX, sizeY, sizeZ};

Chunk::Chunk(const Point3i &worldPos)
  : mWorldPos(worldPos)
{ }
Chunk::Chunk(const Point3i &worldPos, const std::array<cBlock, Chunk::totalSize> &data)
  : Chunk(worldPos)
{
  mData = data;
  for(auto &b : mData)
    {
      if(b.type != block_t::NONE)
        { mNumBlocks++; }
    }
  mDirty = true;
}

bool Chunk::isEmpty() const
{
  //LOGD("CHUNK BLOCKS: %d", (int)mNumBlocks);
  return mNumBlocks == 0;
}


cBlock* Chunk::at(int bx, int by, int bz)
{ return &mData[mIndexer.index(bx, by, bz)]; }
cBlock* Chunk::at(const Point3i &bp)
{ return &mData[mIndexer.index(bp)]; }
block_t Chunk::get(int bx, int by, int bz) const
{ return mData[mIndexer.index(bx, by, bz)].type; }
block_t Chunk::get(const Point3i &bp) const
{ return mData[mIndexer.index(bp[0], bp[1], bp[2])].type; }
std::array<cBlock, Chunk::totalSize>& Chunk::data()
{ return mData; }
const std::array<cBlock, Chunk::totalSize>& Chunk::data() const
{ return mData; }

bool Chunk::set(int bx, int by, int bz, block_t type)
{
  cBlock &b = mData[mIndexer.index(bx, by, bz)];
  if(type != b.type)
    {
      b.type = type;
      if(type == block_t::NONE)
        { mNumBlocks--; }
      else
        { mNumBlocks++; }
      return true;
    }
  else
    { return false; }
}
bool Chunk::set(const Point3i &bp, block_t type)
{
  cBlock &b = mData[mIndexer.index(bp)];
  if(type != b.type)
    {
      b.type = type;
      if(type == block_t::NONE)
        { mNumBlocks--; }
      else
        { mNumBlocks++; }
      return true;
    }
  else
    { return false; }
}
void Chunk::setData(const std::array<cBlock, Chunk::totalSize> &data)
{
  mNumBlocks = 0;
  mData = data;
  for(auto &b : mData)
    {
      if(b.type != block_t::NONE)
        { mNumBlocks++; }
    }
  mDirty = true;
}

// TODO: Run length encoding
int Chunk::serialize(uint8_t *dataOut) const
{
  int size = 0;
  for(const auto &block : mData)
    {
      block.serialize(dataOut + size);
      size += cBlock::dataSize;
    }
  return size;
}
void Chunk::deserialize(const uint8_t *dataIn, int bytes)
{
  int offset = 0;
  mNumBlocks = 0;
  for(auto &block : mData)
    {
      block.deserialize((dataIn + offset), cBlock::dataSize);
      if(block.type != block_t::NONE)
        { mNumBlocks++; }
      
      offset += cBlock::dataSize;
      if(offset >= bytes)
        { break; }
    }
  mDirty = true;
}



/*
void Chunk::updateBlock(const Point3i &bp)
{
  int bi = mIndexer.index(bp);
  mData[bi].updateOcclusion();
  //LOGD("B: %d, %d, %d : BI: %d", bp[0], bp[1], bz, bi);
  if(bp[0] < sizeX - 1)
    {
      mData[bi].setActive(blockSide_t::PX, mData[shiftPX(bi)].type);
    }
  if(bp[1] < sizeY - 1)
    {
      mData[bi].setActive(blockSide_t::PY, mData[shiftPY(bi)].type);
    }
  if(bp[2] < sizeZ - 1)
    {
      mData[bi].setActive(blockSide_t::PZ, mData[shiftPZ(bi)].type);
    }
  if(bp[0] > 0)
    {
      mData[bi].setActive(blockSide_t::NX, mData[shiftNX(bi)].type);
    }
  if(bp[1] > 0)
    {
      mData[bi].setActive(blockSide_t::NY, mData[shiftNY(bi)].type);
    }
  if(bp[2] > 0)
    {
      mData[bi].setActive(blockSide_t::NZ, mData[shiftNZ(bi)].type);
    }
  mDirty = true;
}

void Chunk::updateAllBlocks()
{
  for(int by = 0; by < sizeY; by++)
    for(int bz = 0; bz < sizeZ; bz++)
      for(int bx = 0; bx < sizeX; bx++)
        {
          updateBlock(Point3i{bx, by, bz});
        }
    
}


void Chunk::markDirtyEdges(blockSide_t edge)
{
  if(edge != blockSide_t::NONE)
    {
      const blockSide_t xEdge = edge & (blockSide_t::PX | blockSide_t::NX);
      const blockSide_t yEdge = edge & (blockSide_t::PY | blockSide_t::NY);
      const blockSide_t zEdge = edge & (blockSide_t::PZ | blockSide_t::NZ);
      const blockSide_t xyEdge = edge & (blockSide_t::PX | blockSide_t::NX | blockSide_t::PZ | blockSide_t::NZ);
      const blockSide_t yzEdge = edge & (blockSide_t::PY | blockSide_t::NY | blockSide_t::PZ | blockSide_t::NZ);
      const blockSide_t xzEdge = edge & (blockSide_t::PX | blockSide_t::NX | blockSide_t::PZ | blockSide_t::NZ);
      if(xEdge != blockSide_t::NONE)
        {
          //LOGD("X EDGE DIRTY");
          mDirtyEdges[xEdge] = true;
      //LOGD("X EDGES DONE");
        }
      if(yEdge != blockSide_t::NONE)
        {
          //LOGD("Y EDGE DIRTY (%d)");
          mDirtyEdges[yEdge] = true;
          
          //LOGD("Y EDGES DONE");
        }
      if(zEdge != blockSide_t::NONE)
        {
          //LOGD("Z EDGE DIRTY");
          mDirtyEdges[zEdge] = true;
      //LOGD("Z EDGES DONE");
        }
      if(xzEdge != blockSide_t::NONE)
        { mDirtyEdges[xzEdge] = true; }
      if(yzEdge != blockSide_t::NONE)
        { mDirtyEdges[yzEdge] = true; }
      if(xzEdge != blockSide_t::NONE)
        { mDirtyEdges[xzEdge] = true; }
      
      // if(xEdge != blockSide_t::NONE && yEdge != blockSide_t::NONE)
      //   { mDirtyEdges[xEdge | yEdge] = true; }
      // if(xEdge != blockSide_t::NONE && zEdge != blockSide_t::NONE)
      //   { mDirtyEdges[xEdge | zEdge] = true; }
      // if(yEdge != blockSide_t::NONE && zEdge != blockSide_t::NONE)
      //   { mDirtyEdges[yEdge | zEdge] = true; }
      // if(xEdge != blockSide_t::NONE && yEdge != blockSide_t::NONE && zEdge != blockSide_t::NONE)
      //   { mDirtyEdges[xEdge | yEdge | zEdge] = true; }
      
    }
}

void Chunk::updateCenter()
{
  //if(!mDirty)
  //{ return; }
  
  mDirty = false;
  int bi = 0;
  for(int by = 0; by < sizeY; by++)
    for(int bz = 0; bz < sizeZ; bz++)
      for(int bx = 0; bx < sizeX; bx++, bi++)
        {
          mData[bi].updateOcclusion();
          //LOGD("B: %d, %d, %d : BI: %d", bx, by, bz, bi);
          if(bx < sizeX - 1)
            {
              mData[bi].setActive(blockSide_t::PX, mData[shiftPX(bi)].type);
            }
          if(by < sizeY - 1)
            {
              mData[bi].setActive(blockSide_t::PY, mData[shiftPY(bi)].type);
            }
          if(bz < sizeZ - 1)
            {
              mData[bi].setActive(blockSide_t::PZ, mData[shiftPZ(bi)].type);
            }
          if(bx > 0)
            {
              mData[bi].setActive(blockSide_t::NX, mData[shiftNX(bi)].type);
            }
          if(by > 0)
            {
              mData[bi].setActive(blockSide_t::NY, mData[shiftNY(bi)].type);
            }
          if(bz > 0)
            {
              mData[bi].setActive(blockSide_t::NZ, mData[shiftNZ(bi)].type);
            }
        }
}

void Chunk::updateEdge(Chunk *neighbor, blockSide_t edge)
{
  //if(!mDirtyEdges[edge])
  //{ return; }
  
  //LOGD("UPDATING EDGE (%d)", (int)edge);
  mDirtyEdges[edge] = false;
  //LOGD("TEST");
  Point3i dir = sideDirection(edge);
  int dimension = std::abs(dir[0]) + std::abs(dir[1]) + std::abs(dir[2]);
  //LOGD("%d", dimension);
  if(dimension == 1)
    { // 2D update
      //LOGD("SQUARE TOUCHING");
      const int d = (dir[0]!=0 ? 0 : (dir[1]!=0 ? 1 : 2));
      const int d1 = (d+1)%3;
      const int d2 = (d+2)%3;
      
      Point3i bp;
      bp[d] = (dir[d] < 0 ? 0 : size[d]-1);
      for(bp[d1] = 0; bp[d1] < size[d1]; bp[d1]++)
        for(bp[d2] = 0; bp[d2] < size[d2]; bp[d2]++)
          {
            Point3i np = bp;
            np[d] = (np[d] + size[d]) % size[d];
            cBlock *b = at(bp);
            b->setActive(edge, neighbor->at(np)->type);
            neighbor->at(np)->setActive(oppositeSide(edge), b->type);
          }
    }
  else if(dimension == 2)
    { // 1D update
      // just update lighting (TODO)
      //LOGD("LINE TOUCHING");
      
      for(int d = 0; d < 3; d++)
        {
          if(dir[d] == 0)
            {
              Point3i p = size;
              for(p[d] = 0; p[d] < size[d]; p[d]++)
                { at(p)->updateOcclusion(); }
              break;
            }
        }
    }
  else if(dimension == 3)
    { // single-block update
      // just update lighting (TODO)
      //LOGD("POINT TOUCHING");
      Point3i bp{(dir[0] < 0 ? 0 : sizeX-1),
                 (dir[1] < 0 ? 0 : sizeY-1),
                 (dir[2] < 0 ? 0 : sizeZ-1) };
      cBlock *b = at(bp);
      b->updateOcclusion();
    }
}
void Chunk::updateEdges(const std::unordered_map<blockSide_t, Chunk*> &neighbors)
{
  for(auto &iter : neighbors)//neighbors)
    {
      mDirtyEdges[iter.first] = false;
    }
  for(int by = 0; by < sizeY; by++)
    for(int bz = 0; bz < sizeZ; bz++)
      for(int bx = 0; bx < sizeX; bx++)
        {
          blockSide_t edges = chunkEdge(bx, by, bz);
          if(edges != blockSide_t::NONE)
            {
              cBlock *b = at(bx, by, bz);
              b->updateOcclusion();
              
              for(int i = 0; i < 3; i++)
                {
                  blockSide_t dSide = (i==0 ? (edges & (blockSide_t::PX | blockSide_t::NX)) :
                                       (i==1 ? (edges & (blockSide_t::PY | blockSide_t::NY)) :
                                        (edges & (blockSide_t::PZ | blockSide_t::NZ))));
              
                  if(dSide != blockSide_t::NONE)
                    {
                      const auto &iter = neighbors.find(dSide);
                      if(iter != neighbors.end())
                        {
                          Point3i dir = sideDirection(dSide);
                          Point3i np = Point3i{(bx + dir[0] + sizeX) % sizeX,
                                               (by + dir[1] + sizeY) % sizeY,
                                               (bz + dir[2] + sizeZ) % sizeZ };
                          cBlock *nb = iter->second->at(np);
                          nb->updateOcclusion();
                          b->setActive(dSide, nb->type);
                          nb->setActive(oppositeSide(dSide), b->type);
                          //iter->second->setDirty(true);
                          
                        }
                    }
                }

              
              int bi = mIndexer.index(bx, by, bz);
              if(bx < sizeX-1)
                {
                  b->setActive(blockSide_t::PX, mData[shiftPX(bi)].type);
                  mData[shiftPX(bi)].setActive(blockSide_t::NX, b->type);
                }
              if(bx > 0)
                {
                  b->setActive(blockSide_t::NX, mData[shiftNX(bi)].type);
                  mData[shiftNX(bi)].setActive(blockSide_t::PX, b->type);
                }
              if(by < sizeY-1)
                {
                  b->setActive(blockSide_t::PY, mData[shiftPY(bi)].type);
                  mData[shiftPY(bi)].setActive(blockSide_t::NY, b->type);
                }
              if(by > 0)
                {
                  b->setActive(blockSide_t::NY, mData[shiftNY(bi)].type);
                  mData[shiftNY(bi)].setActive(blockSide_t::PY, b->type);
                }
              if(bz < sizeZ-1)
                {
                  b->setActive(blockSide_t::PZ, mData[shiftPZ(bi)].type);
                  mData[shiftPZ(bi)].setActive(blockSide_t::NZ, b->type);
                }
              if(bz > 0)
                {
                  b->setActive(blockSide_t::NZ, mData[shiftNZ(bi)].type);
                  mData[shiftNZ(bi)].setActive(blockSide_t::PZ, b->type);
                }

              
              // // update diagonal neighbors 
              // blockSide_t xSide = (edges & (blockSide_t::PX | blockSide_t::NX));
              // blockSide_t ySide = (edges & (blockSide_t::PY | blockSide_t::NY));
              // blockSide_t zSide = (edges & (blockSide_t::PZ | blockSide_t::NZ));

              // if(xSide != blockSide_t::NONE)
              //   {
              //     if(ySide != blockSide_t::NONE)
              //       {
              //         if(mNeighbors[xSide | ySide])
              //           { mNeighbors[xSide | ySide]->setMeshDirty(true); }
                  
              //         if(zSide != blockSide_t::NONE && mNeighbors[xSide | ySide | zSide])
              //           { mNeighbors[xSide | ySide | zSide]->setMeshDirty(true); }
              //       }
              //     if(zSide != blockSide_t::NONE && mNeighbors[xSide | zSide])
              //       { mNeighbors[xSide | zSide]->setMeshDirty(true); }
              //   }
              // if(ySide != blockSide_t::NONE && zSide != blockSide_t::NONE && mNeighbors[ySide | zSide])
              //   { mNeighbors[ySide | zSide]->setMeshDirty(true); }
              
            }
        }
}
*/
