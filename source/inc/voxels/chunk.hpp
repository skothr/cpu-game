#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "block.hpp"
#include "vector.hpp"
#include "indexing.hpp"
#include "meshing.hpp"
#include <array>
#include <queue>
#include <unordered_map>
#include <unordered_set>

// NOTE:
//  - wx/wy/wz denotes block position within world
//  - bx/by/bz denotes block position within chunk

class Chunk
{
public:
  // size
  static const int shiftX = 6;
  static const int shiftY = 6;
  static const int shiftZ = 6;
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

  bool calcBounds()
  { return mBounds.calcBounds(this); }
  ChunkBounds* getBounds()
  { return &mBounds; }
  
  void setWorldPos(const Point3i &pos)
  { mWorldPos = pos; }
  Point3i pos() const { return mWorldPos; }
  bool isEmpty() const;

  // data access
  block_t operator[](const Point3i &bp) const
  { return mBlocks[mIndexer.index(bp)]; }
  block_t* at(int bx, int by, int bz);
  block_t* at(const Point3i &bp);
  block_t* at(int bi);
  block_t getType(int bx, int by, int bz) const;
  block_t getType(const Point3i &bp) const;
  std::array<block_t, totalSize>& data();
  const std::array<block_t, totalSize>& data() const;

  std::unordered_map<int, ComplexBlock*>& getComplex()
  { return mComplex; }
  
  // setting
  bool setBlock(int bx, int by, int bz, block_t type);
  bool setBlock(const Point3i &bp, block_t type);
  
  bool setComplex(int bx, int by, int bz, CompleteBlock block);
  bool setComplex(const Point3i &bp, CompleteBlock block);

  Chunk* getNeighbor(blockSide_t side);
  void setNeighbor(blockSide_t side, Chunk *neighbor);
  void unsetNeighbor(blockSide_t side);

  void update();

  void floodFill(std::queue<Point3i> &points,
                 std::array<bool, totalSize> &traversed,
                 blockSide_t &sides );
  void updateConnected();
  bool edgesConnected(blockSide_t prevSide, blockSide_t nextSide);
  void printEdgeConnections();
  
  // updating
  bool isDirty() { return mDirty; }
  void setDirty(bool dirty) { mDirty = dirty; }
  bool needsSave() { return mNeedSave; }
  void setNeedSave(bool needSave) { mNeedSave = needSave; }
  bool isReady() { return mReady; }
  void setReady(bool ready) { mReady = ready; }

  // serialization
  int serialize(std::vector<uint8_t> &dataOut) const;
  void deserialize(const std::vector<uint8_t> &dataIn);

  static const Indexer<sizeX, sizeY, sizeZ>& indexer() { return mIndexer; }
  
private:
  static const Indexer<sizeX, sizeY, sizeZ> mIndexer;
  
  Point3i mWorldPos;
  ChunkBounds mBounds;
  std::array<block_t, totalSize> mBlocks;
  std::unordered_map<int, ComplexBlock*> mComplex;
  std::unordered_map<blockSide_t, Chunk*> mNeighbors;
  
  std::atomic<int> mNumBlocks = 0;
  std::atomic<bool> mDirty = true;
  std::atomic<bool> mNeedSave = false;
  std::atomic<bool> mReady = false;

  //std::unordered_map<blockSide_t, blockSide_t> mConnectedEdges;
  uint16_t mConnectedEdges = 0;
  
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
