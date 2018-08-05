#include "chunk.hpp"

#include <iostream>


// class definitions
cChunk::cChunk()
  : cChunk(Point3i())
{ }

cChunk::cChunk(const Point3i &worldPos)
  : mWorldPos(worldPos),
    mNeighbors{{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}}
{ }

cChunk::cChunk(const Point3i &worldPos, const data_t &data)
  : mWorldPos(worldPos), mData(data)
{
  
}

cChunk::cChunk(const cChunk &other)
  : mWorldPos(other.mWorldPos), mData(other.mData)
{ }

Point3i cChunk::pos() const
{ return mWorldPos; }

bool cChunk::empty() const
{
  for(const auto &block : mData)
    { if(block.active()) return false; }
  return true;
}


cBlock* cChunk::at(int bx, int by, int bz)
{ return &mData[index(bx, by, bz)]; }

block_t cChunk::get(int bx, int by, int bz) const
{ return mData[index(bx, by, bz)].type; }
block_t cChunk::get(const Point3i &bp) const
{ return mData[index(bp[0], bp[1], bp[2])].type; }
//cBlock* cChunk::get(int bx, int by, int bz)
//{ return &mData[index(bx, by, bz)]; }

cChunk::data_t& cChunk::data()
{ return mData; }
const cChunk::data_t& cChunk::data() const
{ return mData; }


void cChunk::setNeighbor(normal_t side, cChunk *chunk)
{
  //std::cout << "SET NEIGHBOR --> side: " << (int)side << ", " << pos() << " -> " << chunk->pos() << "\n";
  //int sideInt = (int)side;
  //int numShift = 0;
  //while((int)sideInt > 0)
  //{
  //  sideInt >>= 1;
  //  numShift++;
  //}
  mNeighbors[(int)side] = chunk;
  //mNeighbors[(int)numShift]->mNeighbors[(numShift + 3) % 6] = this;
}
void cChunk::unsetNeighbor(normal_t side)
{
  //int sideInt = (int)side;
  //int numShift = 0;
  //while((int)sideInt > 1)
  //{
  //  sideInt >>= 1;
  //  numShift++;
  //}
  mNeighbors[(int)side] = nullptr;
}

blockSide_t cChunk::chunkEdge(int bx, int by, int bz) const
{
  return ((bx == 0 ? blockSide_t::NX : (bx == sizeX-1 ? blockSide_t::PX : blockSide_t::NONE)) |
	  (by == 0 ? blockSide_t::NY : (by == sizeY-1 ? blockSide_t::PY : blockSide_t::NONE)) |
	  (bz == 0 ? blockSide_t::NZ : (bz == sizeZ-1 ? blockSide_t::PZ : blockSide_t::NONE)));
}
	  
blockSide_t cChunk::set(int bx, int by, int bz, block_t type)
{
  const int bi = index(bx, by, bz);
  if(type != mData[bi].type)
    {
      std::cout << "BLOCK POS: " << bx << ", " << by << ", " << bz << "\n";
      const block_t oldType = mData[bi].type;
      mData[bi].type = type;
      //if((oldType == block_t::NONE) != (type == block_t::NONE))
        {
          if(bx == 0 && mNeighbors[(int)normal_t::NX])
            {
              cBlock &b = mNeighbors[(int)normal_t::NX]->mData[index(sizeX-1, by, bz)];
              b.activeSides |= blockSide_t::PX | blockSide_t::NX;
              //if(type == block_t::NONE) { b.activeSides |= blockSide_t::PX; }
              //else { b.activeSides &= ~blockSide_t::PX; }
            }
          if(by == 0 && mNeighbors[(int)normal_t::NY])
            {
              cBlock &b = mNeighbors[(int)normal_t::NY]->mData[index(bx, sizeY-1, bz)];
              b.activeSides |= blockSide_t::PY | blockSide_t::NY;
              //if(type == block_t::NONE) { b.activeSides |= blockSide_t::PY; }
              //else { b.activeSides &= ~blockSide_t::PY; }
            }
          if(bz == 0 && mNeighbors[(int)normal_t::NZ])
            {
              cBlock &b = mNeighbors[(int)normal_t::NZ]->mData[index(bx, by, sizeZ-1)];
              b.activeSides |= blockSide_t::PZ | blockSide_t::NZ;
              //if(type == block_t::NONE) { b.activeSides |= blockSide_t::PY; }
              //else { b.activeSides &= ~blockSide_t::PZ; }
            }
          if(bx == sizeX-1 && mNeighbors[(int)normal_t::PX])
            {
              cBlock &b = mNeighbors[(int)normal_t::PX]->mData[index(0, by, bz)];
              b.activeSides |= blockSide_t::NX | blockSide_t::PX;
              //if(type == block_t::NONE) { b.activeSides |= blockSide_t::NX; }
              //else { b.activeSides &= ~blockSide_t::NX; }
            }
          if(by == sizeY-1 && mNeighbors[(int)normal_t::PY])
            {
              cBlock &b = mNeighbors[(int)normal_t::PY]->mData[index(bx, 0, bz)];
              b.activeSides |= blockSide_t::NY | blockSide_t::PY;
              //if(type == block_t::NONE) { b.activeSides |= blockSide_t::NY; }
              //else { b.activeSides &= ~blockSide_t::NY; }
           }
          if(bz == sizeZ-1 && mNeighbors[(int)normal_t::PZ])
            {
              cBlock &b = mNeighbors[(int)normal_t::PZ]->mData[index(bx, by, 0)];
              b.activeSides |= blockSide_t::NZ | blockSide_t::PZ;
              //if(type == block_t::NONE) { b.activeSides |= blockSide_t::NZ; }
              //else { b.activeSides &= ~blockSide_t::NZ; }
            }
          updateSides(bx, by, bz, bi);
          //mData[bi].activeSides = ~mData[bi].activeSides;
        }
      
        mDirty = true;
      
      //std::cout << "CHUNK CHANGED: " << (int)mData[bi].type << " --> " << (int)type << " active: " << (int)mData[bi].activeSides << "\n";;
      /*
        if(type == block_t::NONE)
	{ updateInactive(bx, by, bz, bi); }
        else
	{ updateActive(bx, by, bz, bi); }
      */
      //updateActive(bx, by, bz, bi);

          //mData[bi].activeSides |= chunkEdge(bx, by, bz);
          return chunkEdge(bx, by, bz);
        }
      else
        { return blockSide_t::NONE; }
    }
  blockSide_t cChunk::set(const Point3i &bp, block_t type)
  {
    return set(bp[0], bp[1], bp[2], type);
  }
  void cChunk::setData(const data_t &data)
  {
  mData = data;
  mDirty = true;
}


bool cChunk::dirty() const
{
  return mDirty;
}

void cChunk::setClean()
{
  mDirty = false;
}
void cChunk::setDirty()
{
  mDirty = true;
}

void cChunk::updateBlocks()
{
  int bi = 0;
  for(int by = 0; by < sizeY; by++)
    for(int bz = 0; bz < sizeZ; bz++)
      for(int bx = 0; bx < sizeX; bx++, bi++)
        {
          //std::cout << (int)mData[bi].type << "|" << (int)mData[bi].activeSides << " ";
          updateSides(bx, by, bz, bi);
          /*
            if(mData[bi].type != block_t::NONE)
            { updateActive(bx, by, bz, bi); }
            else
            { updateInactive(bx, by, bz, bi); }
          */
        }
}

// TODO: Run length encoding
int cChunk::serialize(uint8_t *dataOut) const
{
  int size = 0;
  for(const auto &block : mData)
    {
      block.serialize(dataOut + size);
      size += cBlock::dataSize;
    }
  return size;
}
void cChunk::deserialize(const uint8_t *dataIn, int bytes)
{
  int offset = 0;
  for(auto &block : mData)
    {
      block.deserialize((dataIn + offset), cBlock::dataSize);
      offset += cBlock::dataSize;
      if(offset + cBlock::dataSize >= bytes)
        { break; }
    }

  // update active sides
  //updateBlocks();
}

std::string cChunk::toString() const
{
  return "CHUNK (TODO: cChunk.toString())";
}

std::ostream& operator<<(std::ostream &os, const cChunk &chunk)
{
  os << "CHUNK (TODO: cChunk::operator<<)";
  return os;
}

#define NEIGHBOR_TEX false

inline void cChunk::updateSides(int bx, int by, int bz)
{ // updates neighboring blocks while updating this block.
  updateSides(bx, by, bz, index(bx, by, bz));
}
inline void cChunk::updateSides(int bx, int by, int bz, int bi)
{ // updates neighboring blocks while updating this block.

  // std::cout << "NEIGHBORS:\n";
  // std::cout << "    " << (long)mNeighbors[0] << "\n";
  // std::cout << "    " << (long)mNeighbors[1] << "\n";
  // std::cout << "    " << (long)mNeighbors[2] << "\n";
  // std::cout << "    " << (long)mNeighbors[3] << "\n";
  // std::cout << "    " << (long)mNeighbors[4] << "\n";
  // std::cout << "    " << (long)mNeighbors[5] << "\n";
  
  //std::cout << "UPDATING CHUNK: " << bx << ", " << by << ", " << bz << " : " << bi << "...\n";
  blockSide_t newSides = blockSide_t::NONE;
  if(bx == sizeX-1 || mData[shiftPX(bi)].type != block_t::NONE)
    {
      //std::cout << "PX  " << bx << "  " << (long)mNeighbors[(int)normal_t::PX] << "\n";
      newSides |= blockSide_t::PX;

      if(bx < sizeX-1)
        {
          //std::cout << "PX block\n";
          if(mData[bi].type != block_t::NONE && mData[shiftPX(bi)].type != block_t::NONE)
            { mData[shiftPX(bi)].activeSides &= ~blockSide_t::NX; }
          else
            { mData[shiftPX(bi)].activeSides |= blockSide_t::NX; }
        }
#if NEIGHBOR_TEST
      else if(mNeighbors[(int)normal_t::PX])
        {
          //std::cout << "PX neighbor\n";
          if(mData[bi].type != block_t::NONE && mData[shiftPX(bi)].type != block_t::NONE)
            { mNeighbors[(int)normal_t::PX]->mData[(shiftPX(bi)+totalSize)%totalSize].activeSides &= ~blockSide_t::NX; }
          else
            { mNeighbors[(int)normal_t::PX]->mData[(shiftPX(bi)+totalSize)%totalSize].activeSides |= blockSide_t::NX; }
        }
#endif
    }
  else if(bx == 0 || mData[shiftNX(bi)].type != block_t::NONE)
    {
      //std::cout << "NX  "<< bx << "  "  << (long)mNeighbors[(int)normal_t::NX] << "\n";
      newSides |= blockSide_t::NX;
      
      if(bx > 0)
        {
          //std::cout << "NX block\n";
          if(mData[bi].type != block_t::NONE && mData[shiftNX(bi)].type != block_t::NONE)
            { mData[shiftNX(bi)].activeSides &= ~blockSide_t::PX; }
          else
            { mData[shiftNX(bi)].activeSides |= blockSide_t::PX; }
        }
#if NEIGHBOR_TEST
      else if(mNeighbors[(int)normal_t::NX])
        {
          //std::cout << "NX neighbor: " << (long)mNeighbors[(int)normal_t::NX] << "\n";
          //std::cout << "  index: " << (int)(shiftNX(bi)+sizeX)%sizeX << "\n";
          //std::cout << (int)mNeighbors[(int)normal_t::NX]->mData[(shiftNX(bi)+sizeX)%sizeX].type << "\n";

          if(mData[bi].type != block_t::NONE)
            { mNeighbors[(int)normal_t::NX]->mData[(shiftNX(bi)+totalSize)%totalSize].activeSides &= ~blockSide_t::NX; }
          else
            { mNeighbors[(int)normal_t::NX]->mData[(shiftNX(bi)+totalSize)%totalSize].activeSides |= blockSide_t::NX; }
        }
#endif
    }
  //std::cout << "NOW Y...\n";
  if(by == sizeY-1 || mData[shiftPY(bi)].type != block_t::NONE)
    {
      //std::cout << "PY  " << (long)mNeighbors[(int)normal_t::PY] << "\n";
      newSides |= blockSide_t::PY;
      
      if(by < sizeY-1)
        {
          //std::cout << "PY block\n";
          if(mData[bi].type != block_t::NONE && mData[shiftPY(bi)].type != block_t::NONE)
            { mData[shiftPY(bi)].activeSides &= ~blockSide_t::NY; }
          else
            { mData[shiftPY(bi)].activeSides |= blockSide_t::NY; }
        }
#if NEIGHBOR_TEST
      else if(mNeighbors[(int)normal_t::PY])
        {
          //std::cout << "PY neighbor\n";
          if(mData[bi].type != block_t::NONE)
            { mNeighbors[(int)normal_t::PY]->mData[(shiftPY(bi)+totalSize)%totalSize].activeSides &= ~blockSide_t::NY; }
          else
            { mNeighbors[(int)normal_t::PY]->mData[(shiftPY(bi)+totalSize)%totalSize].activeSides |= blockSide_t::NY; }
        }
#endif
    }
  else if(by == 0 || mData[shiftNY(bi)].type != block_t::NONE)
    {
      //std::cout << "NY  " << (long)mNeighbors[(int)normal_t::NY] << "\n";
      newSides |= blockSide_t::NY;
      
      if(by > 0)
        {
          //std::cout << "NY block\n";
          if(mData[bi].type != block_t::NONE && mData[shiftNY(bi)].type != block_t::NONE)
            { mData[shiftNY(bi)].activeSides &= ~blockSide_t::PY; }
          else
            { mData[shiftNY(bi)].activeSides |= blockSide_t::PY; }
        }
#if NEIGHBOR_TEST
      else if(mNeighbors[(int)normal_t::NY])
        {
          //std::cout << "NY neighbor\n";
          if(mData[bi].type != block_t::NONE)
            { mNeighbors[(int)normal_t::NY]->mData[(shiftNY(bi)+totalSize)%totalSize].activeSides &= ~blockSide_t::PY; }
          else
            { mNeighbors[(int)normal_t::NY]->mData[(shiftNY(bi)+totalSize)%totalSize].activeSides |= blockSide_t::PY; }
        }
#endif
    }
  //std::cout << "NOW Z...\n";
  
  if(bz == sizeZ-1 || mData[shiftPZ(bi)].type != block_t::NONE)
    {
      //std::cout << "Z  " << (long)mNeighbors[(int)normal_t::PZ] << "\n";
      newSides |= blockSide_t::PZ;
      
      if(bz < sizeZ-1)
        {
          //std::cout << "PZ block\n";
          if(mData[bi].type != block_t::NONE && mData[shiftPZ(bi)].type != block_t::NONE)
            { mData[shiftPZ(bi)].activeSides &= ~blockSide_t::NZ; }
          else
            { mData[shiftPZ(bi)].activeSides |= blockSide_t::NZ; }
        }
#if NEIGHBOR_TEST
      else if(mNeighbors[(int)normal_t::PZ])
        {
          //std::cout << "PZ neighbor\n";
          if(mData[bi].type != block_t::NONE)
            { mNeighbors[(int)normal_t::PZ]->mData[(shiftPZ(bi)+totalSize)%totalSize].activeSides &= ~blockSide_t::NZ; }
          else
            { mNeighbors[(int)normal_t::PZ]->mData[(shiftPZ(bi)+totalSize)%totalSize].activeSides |= blockSide_t::NZ; }
        }
#endif
    }
  else if(bz == 0 || mData[shiftNZ(bi)].type != block_t::NONE)
    {
      //std::cout << "Z  " << (long)mNeighbors[(int)normal_t::NZ] << "\n";
      newSides |= blockSide_t::NZ;

      if(bz > 0)
        {
          //std::cout << "NZ block\n";
          if(mData[bi].type != block_t::NONE && mData[shiftNZ(bi)].type != block_t::NONE)
            { mData[shiftNZ(bi)].activeSides &= ~blockSide_t::PZ; }
          else
            { mData[shiftNZ(bi)].activeSides |= blockSide_t::PZ; }
        }
#if NEIGHBOR_TEST
      else if(mNeighbors[(int)normal_t::NZ])
        {
          //std::cout << "NZ neighbor\n";
          if(mData[bi].type != block_t::NONE)
            { mNeighbors[(int)normal_t::NZ]->mData[(shiftNZ(bi)+totalSize)%totalSize].activeSides &= ~blockSide_t::PZ; }
          else
            { mNeighbors[(int)normal_t::NZ]->mData[(shiftNZ(bi)+totalSize)%totalSize].activeSides |= blockSide_t::PZ; }
        }
#endif
    }
  std::cout << "DONE --> New: " << (int)newSides << ", old: " << (int)mData[bi].activeSides << "!\n";
  mData[bi].activeSides = ~newSides;
}

inline void cChunk::updateActive(int bx, int by, int bz, int bi)
{ // updates neighboring blocks while updating this block.
  blockSide_t newSides = (((bx == sizeX-1 || !mData[shiftPX(bi)].activeMirror(blockSide_t::NX)) ?
                           blockSide_t::PX : blockSide_t::NONE ) |
                          ((by == sizeY-1 || !mData[shiftPY(bi)].activeMirror(blockSide_t::NY)) ?
                           blockSide_t::PY : blockSide_t::NONE ) |
                          ((bz == sizeZ-1 || !mData[shiftPZ(bi)].activeMirror(blockSide_t::NZ)) ?
                           blockSide_t::PZ : blockSide_t::NONE ) |
                          ((bx == 0 || !mData[shiftNX(bi)].activeMirror(blockSide_t::PX)) ?
                           blockSide_t::NX : blockSide_t::NONE ) |
                          ((by == 0 || !mData[shiftNY(bi)].activeMirror(blockSide_t::PY)) ?
                           blockSide_t::NY : blockSide_t::NONE ) |
                          ((bz == 0 || !mData[shiftNZ(bi)].activeMirror(blockSide_t::PZ)) ?
                           blockSide_t::NZ : blockSide_t::NONE ));
  mData[bi].activeSides = blockSide_t::ALL;//newSides;
}

inline void cChunk::updateInactive(int bx, int by, int bz, int bi)
{ // updates neighboring blocks while updating this block.
  if(bx < sizeX - 1)
    mData[shiftPX(bi)].activeSides |= blockSide_t::NX;
  if(by < sizeY - 1)
    mData[shiftPY(bi)].activeSides |= blockSide_t::NY;
  if(bz < sizeZ - 1)
    mData[shiftPZ(bi)].activeSides |= blockSide_t::NZ;
  if(bx > 0)
    mData[shiftNZ(bi)].activeSides |= blockSide_t::PX;
  if(by > 0)
    mData[shiftNY(bi)].activeSides |= blockSide_t::PY;
  if(bz > 0)
    mData[shiftNZ(bi)].activeSides |= blockSide_t::PZ;

  //mData[bi].activeSides = blockSide_t::ALL;
  /*
    blockSide_t activeSides = (((bx == sizeX-1 || mData[shiftPX(bi)].activeEmpty(blockSide_t::NX)) ?
    blockSide_t::PX : blockSide_t::NONE ) |
    ((by == sizeY-1 || mData[shiftPY(bi)].activeEmpty(blockSide_t::NY)) ?
    blockSide_t::PY : blockSide_t::NONE ) |
    ((bz == sizeZ-1 || mData[shiftPZ(bi)].activeEmpty(blockSide_t::NZ)) ?
    blockSide_t::PZ : blockSide_t::NONE ) |
    ((bx == 0 || mData[shiftNX(bi)].activeEmpty(blockSide_t::PX)) ?
    blockSide_t::NX : blockSide_t::NONE ) |
    ((by == 0 || mData[shiftNY(bi)].activeEmpty(blockSide_t::PY)) ?
    blockSide_t::NY : blockSide_t::NONE ) |
    ((bz == 0 || mData[shiftNZ(bi)].activeEmpty(blockSide_t::PZ)) ?
    blockSide_t::NZ : blockSide_t::NONE ));
    //mData[bi].activeSides = activeSides;
    */
}
