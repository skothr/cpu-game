#ifndef INDEXING_HPP
#define INDEXING_HPP

#include "vector.hpp"

template<int SX, int SY, int SZ>
class Indexer
{
private:
  static const int yMult = SX * SZ;
public:
  Indexer() { }
  
  inline int index(int x, int y, int z) const
  { return x + SX * (z + SZ * y); }
  inline int index(const Point3i &p) const
  { return p[0] + SX * (p[2] + SZ * p[1]); }
  
  inline Point3i unindex(int index) const
  {
    const int yi = index / yMult;
    index -= yi * yMult;
    const int zi = index / SX;
    const int xi = index - zi * SX;
    return Point3i{xi, yi, zi};
  }
  
  inline int shiftPX(int x) const
  { return x + 1; }
  inline int shiftPY(int y) const
  { return y + SX * SZ; }
  inline int shiftPZ(int z) const
  { return z + SX; }
  
  inline int shiftNX(int x) const
  { return x - 1; }
  inline int shiftNY(int y) const
  { return y - SX * SZ; }
  inline int shiftNZ(int z) const
  { return z - SX; }
};


#endif // INDEXING_HPP
