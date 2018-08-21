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
  
private:
  Vector3i mDim;
  cCircularBuffer<cCircularBuffer<cCircularBuffer<cChunk>>> mData;
  
};



#endif // CHUNK_ARRAY_HPP
