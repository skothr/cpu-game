#ifndef FLUID_CHUNK_HPP
#define FLUID_CHUNK_HPP

#include "block.hpp"
#include "blockSides.hpp"
#include "hashing.hpp"
#include "chunk.hpp"
#include "vector.hpp"
#include "fluid.hpp"

#include <atomic>
#include <unordered_map>

class FluidChunk
{
public:
  // size
  static const int shiftX = Chunk::shiftX;
  static const int shiftY = Chunk::shiftY;
  static const int shiftZ = Chunk::shiftZ;
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
  { return Chunk::blockPos(wp); }
  static inline blockSide_t chunkEdge(const Point3i &bp)
  { return Chunk::chunkEdge(bp); }

  FluidChunk(const Point3i &worldPos);
  FluidChunk(const Point3i &worldPos, const std::unordered_map<int32_t, Fluid*> &data);

  void setWorldPos(const Point3i &pos)
  { mWorldPos = pos; }
  Point3i pos() const { return mWorldPos; }
  bool isEmpty() const;
  int numBlocks() const;

  // data access
  Fluid* operator[](const Point3i &bp)
  { return mFluids[Hash::hash(bp)]; }
  Fluid* at(int bx, int by, int bz);
  Fluid* at(const Point3i &bp);
  Fluid* at(int bi);
  std::unordered_map<int32_t, Fluid*>& data();
  const std::unordered_map<int32_t, Fluid*>& data() const;

  // setting
  bool set(int bx, int by, int bz, Fluid *data);
  bool set(const Point3i &bp, Fluid *data);
  
  // updating
  bool isDirty() { return mDirty; }
  void setDirty(bool dirty) { mDirty = dirty; }
  bool needsSave() { return mNeedSave; }
  void setNeedSave(bool needSave) { mNeedSave = needSave; }
  bool isIncomplete() { return mIncomplete; }
  void setIncomplete(bool incomplete) { mIncomplete = incomplete; }

  bool step(bool evap);

  // serialization
  int serialize(std::vector<uint8_t> &dataOut) const;
  void deserialize(const std::vector<uint8_t> &dataIn);

private:
  Point3i mWorldPos;
  std::unordered_map<int32_t, Fluid*> mFluids;
  std::atomic<bool> mDirty = true;
  std::atomic<bool> mNeedSave = false;
  std::atomic<bool> mIncomplete = true;

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


#endif // FLUID_CHUNK_HPP
