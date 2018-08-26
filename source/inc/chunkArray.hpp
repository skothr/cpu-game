#ifndef CHUNK_ARRAY_HPP
#define CHUNK_ARRAY_HPP

#include "vector.hpp"
#include "circularBuffer.hpp"
#include "chunk.hpp"

class cChunk;


class cChunkArray
{
public:
  cChunkArray(const Vector3i &dim, const Point3i &cPos);
  ~cChunkArray();

  cChunk* operator[](const Point3i &p);
  const cChunk* operator[](const Point3i &p) const;
  void rotate(const Vector3i &amount);

  cChunk* staticAccess(int x, int y, int z)
  { return &mData.staticAccess(x).staticAccess(y).staticAccess(z); }
  const cChunk* staticAccess(int x, int y, int z) const
  { return &mData.staticAccess(x).staticAccess(y).staticAccess(z); }
  
private:
  Vector3i mDim;
  Point3i mCenter;
  cCircularBuffer<cCircularBuffer<cCircularBuffer<cChunk>>> mData;

  cChunk* at(const Point3i &p)
  { return &mData[p[0]][p[1]][p[2]]; }
  
};



#endif // CHUNK_ARRAY_HPP
