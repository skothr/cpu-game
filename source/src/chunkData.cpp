#include "chunkData.hpp"

#include <iostream>


// class definitions
cChunkData::cChunkData()
{ }

cChunkData::cChunkData(const data_t &data)
  : mData(data)
{
  
}

bool cChunkData::empty() const
{
  for(const auto &block : mData)
    { if(block.active()) return false; }
  return true;
}


cBlock* cChunkData::at(int bx, int by, int bz)
{ return &mData[index(bx, by, bz)]; }

block_t cChunkData::get(int bx, int by, int bz) const
{ return mData[index(bx, by, bz)].type; }
block_t cChunkData::get(const Point3i &bp) const
{ return mData[index(bp[0], bp[1], bp[2])].type; }


cChunkData::data_t& cChunkData::data()
{ return mData; }
const cChunkData::data_t& cChunkData::data() const
{ return mData; }


blockSide_t cChunkData::chunkEdge(int bx, int by, int bz) const
{
  return ((bx == 0 ? blockSide_t::NX : (bx == sizeX-1 ? blockSide_t::PX : blockSide_t::NONE)) |
	  (by == 0 ? blockSide_t::NY : (by == sizeY-1 ? blockSide_t::PY : blockSide_t::NONE)) |
	  (bz == 0 ? blockSide_t::NZ : (bz == sizeZ-1 ? blockSide_t::PZ : blockSide_t::NONE)));
}

blockSide_t cChunkData::set(int bx, int by, int bz, block_t type)
{
  const int bi = index(bx, by, bz);
  if(type != mData[bi].type)
    {
      mData[bi].type = type;
      mData[bi].updateOcclusion();
      const blockSide_t edge = chunkEdge(bx, by, bz);
      if(!(bool)(edge & blockSide_t::PX))
        { updateSide(bx, by, bz, bi, blockSide_t::PX); }
      if(!(bool)(edge & blockSide_t::PY))
        { updateSide(bx, by, bz, bi, blockSide_t::PY); }
      if(!(bool)(edge & blockSide_t::PZ))
        { updateSide(bx, by, bz, bi, blockSide_t::PZ); }
      if(!(bool)(edge & blockSide_t::NX))
        { updateSide(bx, by, bz, bi, blockSide_t::NX); }
      if(!(bool)(edge & blockSide_t::NY))
        { updateSide(bx, by, bz, bi, blockSide_t::NY); }
      if(!(bool)(edge & blockSide_t::NZ))
        { updateSide(bx, by, bz, bi, blockSide_t::NZ); }
      return edge;
    }
  else
    { return blockSide_t::NONE; }
}
blockSide_t cChunkData::set(const Point3i &bp, block_t type)
{ return set(bp[0], bp[1], bp[2], type); }
void cChunkData::setData(const data_t &data)
{ mData = data; }


void cChunkData::updateBlocks()
{
  int bi = 0;
  for(int by = 0; by < sizeY; by++)
    for(int bz = 0; bz < sizeZ; bz++)
      for(int bx = 0; bx < sizeX; bx++, bi++)
        {
          const blockSide_t edge = chunkEdge(bx, by, bz);
          if(!(bool)(edge & blockSide_t::PX))
            { updateSide(bx, by, bz, bi, blockSide_t::PX); }
          if(!(bool)(edge & blockSide_t::PY))
            { updateSide(bx, by, bz, bi, blockSide_t::PY); }
          if(!(bool)(edge & blockSide_t::PZ))
            { updateSide(bx, by, bz, bi, blockSide_t::PZ); }
          if(!(bool)(edge & blockSide_t::NX))
            { updateSide(bx, by, bz, bi, blockSide_t::NX); }
          if(!(bool)(edge & blockSide_t::NY))
            { updateSide(bx, by, bz, bi, blockSide_t::NY); }
          if(!(bool)(edge & blockSide_t::NZ))
            { updateSide(bx, by, bz, bi, blockSide_t::NZ); }
        }
}

// TODO: Run length encoding
int cChunkData::serialize(uint8_t *dataOut) const
{
  int size = 0;
  for(const auto &block : mData)
    {
      block.serialize(dataOut + size);
      size += cBlock::dataSize;
    }
  return size;
}
void cChunkData::deserialize(const uint8_t *dataIn, int bytes)
{
  int offset = 0;
  for(auto &block : mData)
    {
      block.deserialize((dataIn + offset), cBlock::dataSize);
      offset += cBlock::dataSize;
      if(offset >= bytes)
        { break; }
    }
  for(int bi = 0; bi < totalSize; bi++)
    { mData[bi].updateOcclusion(); }
}

#define NEIGHBOR_TEX false

#define FOR_EACH_3x3(lx, ly, lz, hx, hy, hz, ACTION)    \
  for(int x=lx;x<=hx;x++)                               \
    for(int y=ly;y<=hy;y++)                             \
      for(int z=lz;z<=hz;z++)                           \
        ACTION


void cChunkData::updateOcclusion(int bx, int by, int bz)
{
  int bi = index(bx, by, bz);
  mData[bi].updateOcclusion();
}

inline int getAO(int e1, int e2, int c)
{
  if(e1 == 1 && e2 == 1)
    { return 3; }
  else
    { return (e1 + e2 + c); }
}
uint8_t cChunkData::getLighting(int bx, int by, int bz, int vx, int vy, int vz, blockSide_t side)
{
  if(bx==sizeX-1 || by==sizeY-1 || bz==sizeZ-1 ||
     bx==0 || by==0 || bz==0 )
    { return 1; }
  
  int c0=0, c1=0, lc=0, le0=0, le1=0;
  switch(side)
    {
    case blockSide_t::PX:
      c0 = vy * 2 - 1;
      c1 = vz * 2 - 1;
      lc = mData[index(bx+1, by+c0, bz+c1)].occlusion;
      le0 = mData[index(bx+1, by, bz+c1)].occlusion;
      le1 = mData[index(bx+1, by+c0, bz)].occlusion;
      break;
    case blockSide_t::PY:
      c0 = vx * 2 - 1;
      c1 = vz * 2 - 1;
      lc = mData[index(bx+c0, by+1, bz+c1)].occlusion;
      le0 = mData[index(bx, by+1, bz+c1)].occlusion;
      le1 = mData[index(bx+c0, by+1, bz)].occlusion;
      break;
    case blockSide_t::PZ:
      c0 = vy * 2 - 1;
      c1 = vx * 2 - 1;
      lc = mData[index(bx+c1, by+c0, bz+1)].occlusion;
      le0 = mData[index(bx, by+c0, bz+1)].occlusion;
      le1 = mData[index(bx+c1, by, bz+1)].occlusion;
      break;
    case blockSide_t::NX:
      c0 = vy * 2 - 1;
      c1 = vz * 2 - 1;
      lc = mData[index(bx-1, by+c0, bz+c1)].occlusion;
      le0 = mData[index(bx-1, by, bz+c1)].occlusion;
      le1 = mData[index(bx-1, by+c0, bz)].occlusion;
      break;
    case blockSide_t::NY:
      c0 = vx * 2 - 1;
      c1 = vz * 2 - 1;
      lc = mData[index(bx+c0, by-1, bz+c1)].occlusion;
      le0 = mData[index(bx, by-1, bz+c1)].occlusion;
      le1 = mData[index(bx+c0, by-1, bz)].occlusion;
      break;
    case blockSide_t::NZ:
      c0 = vy * 2 - 1;
      c1 = vx * 2 - 1;
      lc = mData[index(bx+c1, by+c0, bz-1)].occlusion;
      le0 = mData[index(bx, by+c0, bz-1)].occlusion;
      le1 = mData[index(bx+c1, by, bz-1)].occlusion;
      break;
    }
  return mData[index(bx+c0, by+1, bz)].lightLevel - getAO(le0, le1, lc);
}

inline void cChunkData::updateSide(int bx, int by, int bz, blockSide_t side)
{ // updates neighboring blocks while updating this block.
  updateSide(bx, by, bz, index(bx, by, bz), side);
}
inline void cChunkData::updateSide(int bx, int by, int bz, int bi, blockSide_t side)
{ // updates adjacent non-edge blocks while updating this block.
  int sIndex;
  switch(side)
    {
    case blockSide_t::PX:
      sIndex = shiftPX(bi);
      break;
    case blockSide_t::PY:
      sIndex = shiftPY(bi);
      break;
    case blockSide_t::PZ:
      sIndex = shiftPZ(bi);
      break;
    case blockSide_t::NX:
      sIndex = shiftNX(bi);
      break;
    case blockSide_t::NY:
      sIndex = shiftNY(bi);
      break;
    case blockSide_t::NZ:
      sIndex = shiftNZ(bi);
      break;
    }
  mData[bi].setActive(side, mData[sIndex].type);
  mData[sIndex].setActive(oppositeSide(side), mData[bi].type);
}
