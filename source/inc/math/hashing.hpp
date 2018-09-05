#ifndef HASHING_HPP
#define HASHING_HPP

#include <cstdint>
#include "vector.hpp"

typedef int32_t hash_t;
namespace HashUtil
{
  inline int32_t expand(int32_t x)
  {
    x                  &= 0x000003FF;
    x  = (x | (x<<16)) &  0xFF0000FF;
    x  = (x | (x<<8))  &  0x0F00F00F;
    x  = (x | (x<<4))  &  0xC30C30C3;
    x  = (x | (x<<2))  &  0x49249249;
    return x;
  }
  inline int32_t unexpand(int32_t x)
  {
    x                 &= 0x49249249;
    x = (x | (x>>2))  &  0xC30C30C3;
    x = (x | (x>>4))  &  0x0F00F00F;
    x = (x | (x>>8))  &  0xFF0000FF;
    x = (x | (x>>16)) &  0x000003FF;
    return (x << 22) >> 22;
  }
  inline int32_t unhashX(int32_t cx)
  { return unexpand(cx); }
  inline int32_t unhashY(int32_t cy)
  { return unexpand(cy>>1); }
  inline int32_t unhashZ(int32_t cz)
  { return unexpand(cz>>2); }
};
namespace Hash
{
  inline int32_t hash(int32_t x, int32_t y, int32_t z)
  { return HashUtil::expand(x) + (HashUtil::expand(y) << 1) + (HashUtil::expand(z) << 2); }
  inline int32_t hash(const Point3i &p)
  { return hash(p[0], p[1], p[2]); }
  inline Point3i unhash(int32_t x)
  { return Point3i{HashUtil::unhashX(x), HashUtil::unhashY(x), HashUtil::unhashZ(x)}; }
};


#endif HASHING_HPP
