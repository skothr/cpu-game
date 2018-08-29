#ifndef CHUNK_ARRAY_HPP
#define CHUNK_ARRAY_HPP

#include "vector.hpp"
#include "circularBuffer.hpp"
#include "chunk.hpp"

class Chunk;


class ChunkArray
{
public:
  ChunkArray(const Vector3i &dim, const Point3i &cPos);
  ~ChunkArray();

  Chunk* operator[](const Point3i &p);
  const Chunk* operator[](const Point3i &p) const;
  void rotate(const Vector3i &amount);

  Chunk* staticAccess(int x, int y, int z)
  { return &mData.staticAccess(x).staticAccess(y).staticAccess(z); }
  const Chunk* staticAccess(int x, int y, int z) const
  { return &mData.staticAccess(x).staticAccess(y).staticAccess(z); }
  
private:
  Vector3i mDim;
  Point3i mCenter;
  cCircularBuffer<cCircularBuffer<cCircularBuffer<Chunk>>> mData;

  Chunk* at(const Point3i &p)
  { return &mData[p[0]][p[1]][p[2]]; }
  
};



#endif // CHUNK_ARRAY_HPP
