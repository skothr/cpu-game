#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "block.hpp"
#include "vector.hpp"
#include "indexing.hpp"
#include <array>
#include <unordered_map>

// NOTE:
//  - wx/wy/wz denotes block position within world
//  - bx/by/bz denotes block position within chunk

class Chunk
{
public:
  // size
  static const int shiftX = 5;
  static const int shiftY = 5;
  static const int shiftZ = 5;
  static const int sizeX = (1 << shiftX);
  static const int sizeY = (1 << shiftY);
  static const int sizeZ = (1 << shiftZ);
  static const Point3i size;
  static const int totalSize = (sizeX * sizeY * sizeZ);
  
  // masks to determine internal position from world position
  static const int maskX = (sizeX - 1);
  static const int maskY = (sizeY - 1);
  static const int maskZ = (sizeZ - 1);
  
  static inline Point3i blockPos(const Point3i &wp)
  { return Point3i({blockX(wp[0]), blockY(wp[1]), blockZ(wp[2])}); }
  static inline blockSide_t chunkEdge(const Point3i &bp)
  { 
    return ((bp[0] == 0 ? blockSide_t::NX : (bp[0] == sizeX-1 ? blockSide_t::PX : blockSide_t::NONE)) |
            (bp[1] == 0 ? blockSide_t::NY : (bp[1] == sizeY-1 ? blockSide_t::PY : blockSide_t::NONE)) |
            (bp[2] == 0 ? blockSide_t::NZ : (bp[2] == sizeZ-1 ? blockSide_t::PZ : blockSide_t::NONE)));
  }

  Chunk(const Point3i &worldPos);
  Chunk(const Point3i &worldPos, const std::array<cBlock, totalSize> &data);

  void setWorldPos(const Point3i &pos)
  { mWorldPos = pos; }
  Point3i pos() const { return mWorldPos; }
  bool isEmpty() const;

  // data access
  cBlock& operator[](const Point3i &bp)
  { return mData[mIndexer.index(bp)]; }
  const cBlock& operator[](const Point3i &bp) const
  { return mData[mIndexer.index(bp)]; }
  cBlock* at(int bx, int by, int bz);
  cBlock* at(const Point3i &bp);
  block_t get(int bx, int by, int bz) const;
  block_t get(const Point3i &bp) const;
  std::array<cBlock, totalSize>& data();
  const std::array<cBlock, totalSize>& data() const;

  // setting
  bool set(int bx, int by, int bz, block_t type);
  bool set(const Point3i &bp, block_t type);
  void setData(const std::array<cBlock, totalSize> &data);
  
  // updating
  bool isPriority() { return mPriority; }
  void setPriority(bool priority) { mPriority = priority; }
  bool isDirty() { return mDirty; }
  void setDirty(bool dirty) { mDirty = dirty; }
  bool isIncomplete() { return mIncomplete; }
  void setIncomplete(bool incomplete) { mIncomplete = incomplete; }

  // serialization
  int serialize(uint8_t *dataOut) const;
  void deserialize(const uint8_t *dataIn, int bytes);
  
  //void updateBlock(const Point3i &bp);
  //void updateAllBlocks();
  
private:
  static const Indexer<sizeX, sizeY, sizeZ> mIndexer;
  
  std::array<cBlock, totalSize> mData;
  std::atomic<int> mNumBlocks = 0;
  std::atomic<bool> mDirty = true;
  std::atomic<bool> mIncomplete = true;
  std::atomic<bool> mPriority = false;
  Point3i mWorldPos;

  static inline int blockX(int wx)
  { return wx & maskX; }
  static inline int blockY(int wy)
  { return wy & maskY; }
  static inline int blockZ(int wz)
  { return wz & maskZ; }
  
  inline int shiftPX(int bx)
  { return bx + 1; }
  inline int shiftPY(int by)
  { return by + sizeX * sizeZ; }
  inline int shiftPZ(int bz)
  { return bz + sizeX; }
  inline int shiftNX(int bx)
  { return bx - 1; }
  inline int shiftNY(int by)
  { return by - sizeX * sizeZ; }
  inline int shiftNZ(int bz)
  { return bz - sizeX; }
};


#endif // CHUNK_HPP