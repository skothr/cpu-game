#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "block.hpp"
#include "vector.hpp"

#include <array>
#include <string>
#include <ostream>

// NOTE:
//  - wx/wy/wz denotes block position within world
//  - bx/by/bz denotes block position within chunk

class cChunk
{
  friend class cChunkManager;
  
public:
  static const int shiftX = 5;
  static const int shiftY = 5;
  static const int shiftZ = 5;
  static const int sizeX = (1 << shiftX);
  static const int sizeY = (1 << shiftY);
  static const int sizeZ = (1 << shiftZ);
  static const int totalSize = (sizeX * sizeY * sizeZ);
  static const int yMult = sizeX * sizeZ;
  static const int maskX = (sizeX - 1);
  static const int maskY = (sizeY - 1);
  static const int maskZ = (sizeZ - 1);

  typedef std::array<cBlock, totalSize> data_t;

  cChunk();
  cChunk(const Point3i &worldPos);
  cChunk(const Point3i &worldPos, const data_t &data);
  
  cChunk(const cChunk &other);

  void setNeighbor(normal_t side, cChunk *chunk);
  void unsetNeighbor(normal_t side);

  void setWorldPos(const Point3i &newPos)
  { mWorldPos = newPos; }
  Point3i pos() const;
  bool empty() const;

  cBlock* at(int bx, int by, int bz);
  block_t get(int bx, int by, int bz) const;
  block_t get(const Point3i &bp) const;
  //cBlock* get(int bx, int by, int bz);
  data_t& data();
  const data_t& data() const;

  blockSide_t chunkEdge(int bx, int by, int bz) const;
  
  blockSide_t set(int bx, int by, int bz, block_t type);
  blockSide_t set(const Point3i &bp, block_t type);
  void setData(const data_t &data);

  void updateBlocks();
  bool dirty() const;
  void setClean();
  void setDirty();

  int serialize(uint8_t *dataOut) const;
  void deserialize(const uint8_t *dataIn, int bytes);

  std::string toString() const;
  friend std::ostream& operator<<(std::ostream &os, const cChunk &chunk);
  
  static int index(int bx, int by, int bz);
  static Point3i unflattenIndex(int index);
  
private:
  data_t mData;
  Point3i mWorldPos;
  bool mDirty = false;

  std::array<cChunk*, 6> mNeighbors;
  inline void updateSides(int bx, int by, int bz, int bi);
  inline void updateSides(int bx, int by, int bz);
  inline void updateActive(int bx, int by, int bz, int bi);
  inline void updateInactive(int bx, int by, int bz, int bi);

  static int shiftPX(int bx);
  static int shiftPY(int by);
  static int shiftPZ(int bz);
  static int shiftNX(int bx);
  static int shiftNY(int by);
  static int shiftNZ(int bz);
  static int blockX(int wx);
  static int blockY(int wy);
  static int blockZ(int wz);
  static Point3i blockPos(const Point3i &wp);
};

// optimized functions for indexing
inline int cChunk::index(int bx, int by, int bz)
{ return bx + sizeX * (bz + sizeZ * by); }
inline Point3i cChunk::unflattenIndex(int index)
{
  const int yi = index / yMult;
  index -= yi * yMult;
  const int zi = index / sizeX;
  const int xi = index - zi * sizeX;
  return Point3i{xi, yi, zi};
}
inline int cChunk::shiftPX(int bx)
{ return bx + 1; }
inline int cChunk::shiftPY(int by)
{ return by + sizeX * sizeZ; }
inline int cChunk::shiftPZ(int bz)
{ return bz + sizeX; }
inline int cChunk::shiftNX(int bx)
{ return bx - 1; }
inline int cChunk::shiftNY(int by)
{ return by - sizeX * sizeZ; }
inline int cChunk::shiftNZ(int bz)
{ return bz - sizeX; }
inline int cChunk::blockX(int wx)
{ return wx & maskX; }
inline int cChunk::blockY(int wy)
{ return wy & maskY; }
inline int cChunk::blockZ(int wz)
{ return wz & maskZ; }
inline Point3i cChunk::blockPos(const Point3i &wp)
{ return Point3i({blockX(wp[0]), blockY(wp[1]), blockZ(wp[2])}); }



#endif // CHUNK_HPP
