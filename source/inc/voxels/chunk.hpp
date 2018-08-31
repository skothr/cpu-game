#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "block.hpp"
#include "fluid.hpp"
#include "vector.hpp"
#include "indexing.hpp"
#include <array>
#include <unordered_map>
#include <unordered_set>

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
  //Chunk(const Point3i &worldPos, const std::array<Block, totalSize> &data);

  void setWorldPos(const Point3i &pos)
  { mWorldPos = pos; }
  Point3i pos() const { return mWorldPos; }
  bool isEmpty() const;

  // data access
  Block& operator[](const Point3i &bp)
  { return mBlocks[mIndexer.index(bp)]; }
  const Block& operator[](const Point3i &bp) const
  { return mBlocks[mIndexer.index(bp)]; }
  Block* at(int bx, int by, int bz);
  Block* at(const Point3i &bp);
  Block* at(int bi);
  block_t getType(int bx, int by, int bz) const;
  block_t getType(const Point3i &bp) const;
  BlockData* getData(const Point3i &bp);
  std::array<Block, totalSize>& data();
  const std::array<Block, totalSize>& data() const;
  
  //FluidData& getFluid(int bx, int by, int bz);
  //FluidData& getFluid(const Point3i &bp);

  std::unordered_map<int, FluidData*> getFluids();

  // setting
  bool setBlock(int bx, int by, int bz, block_t type, BlockData *data = nullptr);
  bool setBlock(const Point3i &bp, block_t type, BlockData *data = nullptr);
  //void setData(const std::array<Block, totalSize> &data);//, const std::unordered_map<int, FluidData> &fluids);
  
  // updating
  bool isPriority() { return mPriority; }
  void setPriority(bool priority) { mPriority = priority; }
  bool isDirty() { return mDirty; }
  void setDirty(bool dirty) { mDirty = dirty; }
  bool isIncomplete() { return mIncomplete; }
  void setIncomplete(bool incomplete) { mIncomplete = incomplete; }

  bool step(bool evap);

  // serialization
  int serialize(std::vector<uint8_t> &dataOut) const;
  void deserialize(const std::vector<uint8_t> &dataIn);

  static const Indexer<sizeX, sizeY, sizeZ>& indexer() { return mIndexer; }
  
private:
  static const Indexer<sizeX, sizeY, sizeZ> mIndexer;
  
  Point3i mWorldPos;
  std::array<Block, totalSize> mBlocks;
  std::unordered_map<int, FluidData*> mFluids;
  std::unordered_map<int, BlockData*> mActive;
  
  std::atomic<int> mNumBlocks = 0;
  int mNumBytes = totalSize*Block::dataSize;
  
  std::atomic<bool> mDirty = true;
  std::atomic<bool> mIncomplete = true;
  std::atomic<bool> mPriority = false;

  void reset();
  
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
