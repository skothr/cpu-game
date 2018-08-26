#ifndef CHUNK_DATA_HPP
#define CHUNK_DATA_HPP

#include "block.hpp"
#include "vector.hpp"
#include "chunkMesh.hpp"
#include <array>

// NOTE:
//  - wx/wy/wz denotes block position within world
//  - bx/by/bz denotes block position within chunk

class cChunkData
{
  friend class cChunk;
  
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

  cChunkData();
  cChunkData(const data_t &data);

  bool empty() const;

  cBlock* at(int bx, int by, int bz);
  block_t get(int bx, int by, int bz) const;
  block_t get(const Point3i &bp) const;

  cBlock& operator[](int i)
  { return mData[i]; }
  const cBlock& operator[](int i) const
  { return mData[i]; }
  
  data_t& data();
  const data_t& data() const;
  
  blockSide_t chunkEdge(int bx, int by, int bz) const;

  blockSide_t set(int bx, int by, int bz, block_t type);
  blockSide_t set(const Point3i &bp, block_t type);
  void setData(const data_t &data);
  
  int serialize(uint8_t *dataOut) const;
  void deserialize(const uint8_t *dataIn, int bytes);

  uint8_t getLighting(int bx, int by, int bz, int vx, int vy, int vz, blockSide_t side);
  void updateBlocks();
  
  static int index(int bx, int by, int bz);
  static Point3i unflattenIndex(int index);
  static Point3i blockPos(const Point3i &wp);
  
private:
  data_t mData;

  inline void updateSide(int bx, int by, int bz, int bi, blockSide_t side);
  inline void updateSide(int bx, int by, int bz, blockSide_t side);
  
  void updateOcclusion(int bx, int by, int bz);

  static int shiftPX(int bx);
  static int shiftPY(int by);
  static int shiftPZ(int bz);
  static int shiftNX(int bx);
  static int shiftNY(int by);
  static int shiftNZ(int bz);
  static int blockX(int wx);
  static int blockY(int wy);
  static int blockZ(int wz);
};

// optimized functions for indexing
inline int cChunkData::index(int bx, int by, int bz)
{ return bx + sizeX * (bz + sizeZ * by); }
inline Point3i cChunkData::unflattenIndex(int index)
{
  const int yi = index / yMult;
  index -= yi * yMult;
  const int zi = index / sizeX;
  const int xi = index - zi * sizeX;
  return Point3i{xi, yi, zi};
}
inline int cChunkData::shiftPX(int bx)
{ return bx + 1; }
inline int cChunkData::shiftPY(int by)
{ return by + sizeX * sizeZ; }
inline int cChunkData::shiftPZ(int bz)
{ return bz + sizeX; }
inline int cChunkData::shiftNX(int bx)
{ return bx - 1; }
inline int cChunkData::shiftNY(int by)
{ return by - sizeX * sizeZ; }
inline int cChunkData::shiftNZ(int bz)
{ return bz - sizeX; }
inline int cChunkData::blockX(int wx)
{ return wx & maskX; }
inline int cChunkData::blockY(int wy)
{ return wy & maskY; }
inline int cChunkData::blockZ(int wz)
{ return wz & maskZ; }
inline Point3i cChunkData::blockPos(const Point3i &wp)
{ return Point3i({blockX(wp[0]), blockY(wp[1]), blockZ(wp[2])}); }



#endif // CHUNK_DATA_HPP
