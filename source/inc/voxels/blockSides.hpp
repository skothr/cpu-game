#ifndef BLOCK_SIDES_HPP
#define BLOCK_SIDES_HPP

#include <cstdint>
#include <vector>
#include <unordered_map>

#include "helpers.hpp"
#include "vector.hpp"

enum class normal_t : uint8_t
  {
   PX = 0, PY, PZ,
   NX, NY, NZ,
   COUNT
  };

enum class blockSide_t : uint8_t
  {
   NONE = 0x00,
   PX = 0x01, PY = 0x02, PZ = 0x04, NX = 0x08, NY = 0x10, NZ = 0x20,
   ALL = (PX | PY | PZ | NX | NY | NZ),
   MOD_FLAG = (0x40 - 1)
  };
ENUM_CLASS_BITWISE_OPERATORS(blockSide_t)


static std::vector<blockSide_t> gBlockSides =
  { // Z = -1
    blockSide_t::NX | blockSide_t::NY | blockSide_t::NZ,
    blockSide_t::NY | blockSide_t::NZ,
    blockSide_t::PX | blockSide_t::NY | blockSide_t::NZ,
    blockSide_t::NX | blockSide_t::NZ,
    blockSide_t::NZ,
    blockSide_t::PX | blockSide_t::NZ,
    blockSide_t::NX | blockSide_t::PY | blockSide_t::NZ,
    blockSide_t::PY | blockSide_t::NZ,
    blockSide_t::PX | blockSide_t::PY | blockSide_t::NZ,
    // Z = 0
    blockSide_t::NX | blockSide_t::NY,
    blockSide_t::NY,
    blockSide_t::PX | blockSide_t::NY,
    blockSide_t::NX,
    blockSide_t::PX,
    blockSide_t::NX | blockSide_t::PY,
    blockSide_t::PY,
    blockSide_t::PX | blockSide_t::PY,
    // Z = 1
    blockSide_t::NX | blockSide_t::NY | blockSide_t::PZ,
    blockSide_t::NY | blockSide_t::PZ,
    blockSide_t::PX | blockSide_t::NY | blockSide_t::PZ,
    blockSide_t::NX | blockSide_t::PZ,
    blockSide_t::PZ,
    blockSide_t::PX | blockSide_t::PZ,
    blockSide_t::NX | blockSide_t::PY | blockSide_t::PZ,
    blockSide_t::PY | blockSide_t::PZ,
    blockSide_t::PX | blockSide_t::PY | blockSide_t::PZ };

static std::unordered_map<blockSide_t, int> gBlockSideIndices =
  { {blockSide_t::NONE, -1},
    {gBlockSides[0], 0 },{gBlockSides[1], 1 },{gBlockSides[2], 2 },
    {gBlockSides[3], 3 },{gBlockSides[4], 4 },{gBlockSides[5], 5 },
    {gBlockSides[6], 6 },{gBlockSides[7], 7 },{gBlockSides[8], 8 },
    {gBlockSides[9], 9 },{gBlockSides[10],10},{gBlockSides[11],11},
    {gBlockSides[12],12},{gBlockSides[13],13},{gBlockSides[14],14},
    {gBlockSides[15],15},{gBlockSides[16],16},{gBlockSides[17],17},
    {gBlockSides[18],18},{gBlockSides[19],19},{gBlockSides[20],20},
    {gBlockSides[21],21},{gBlockSides[22],22},{gBlockSides[23],23},
    {gBlockSides[24],24},{gBlockSides[25],25} };

// directions from center
static std::unordered_map<blockSide_t, Point3i> gSideDirections =
  { {blockSide_t::NONE, Point3i({0, 0, 0})},
    {gBlockSides[0],  Point3i({-1,-1,-1})},
    {gBlockSides[1],  Point3i({ 0,-1,-1})},
    {gBlockSides[2],  Point3i({ 1,-1,-1})},
    {gBlockSides[3],  Point3i({-1, 0,-1})},
    {gBlockSides[4],  Point3i({ 0, 0,-1})},
    {gBlockSides[5],  Point3i({ 1, 0,-1})},
    {gBlockSides[6],  Point3i({-1, 1,-1})},
    {gBlockSides[7],  Point3i({ 0, 1,-1})},
    {gBlockSides[8],  Point3i({ 1, 1,-1})},
    
    {gBlockSides[9],  Point3i({-1,-1, 0})},
    {gBlockSides[10], Point3i({ 0,-1, 0})},
    {gBlockSides[11], Point3i({ 1,-1, 0})},
    {gBlockSides[12], Point3i({-1, 0, 0})},
    {gBlockSides[13], Point3i({ 1, 0, 0})},
    {gBlockSides[14], Point3i({-1, 1, 0})},
    {gBlockSides[15], Point3i({ 0, 1, 0})},
    {gBlockSides[16], Point3i({ 1, 1, 0})},
    
    {gBlockSides[17], Point3i({-1,-1, 1})},
    {gBlockSides[18], Point3i({ 0,-1, 1})},
    {gBlockSides[19], Point3i({ 1,-1, 1})},
    {gBlockSides[20], Point3i({-1, 0, 1})},
    {gBlockSides[21], Point3i({ 0, 0, 1})},
    {gBlockSides[22], Point3i({ 1, 0, 1})},
    {gBlockSides[23], Point3i({-1, 1, 1})},
    {gBlockSides[24], Point3i({ 0, 1, 1})},
    {gBlockSides[25], Point3i({ 1, 1, 1})} };

// opposite sides
static std::unordered_map<blockSide_t, blockSide_t> gOppositeSides =
  { {blockSide_t::NONE, blockSide_t::NONE},
    {gBlockSides[0],  gBlockSides[25]}, {gBlockSides[1],  gBlockSides[24]},
    {gBlockSides[2],  gBlockSides[23]}, {gBlockSides[3],  gBlockSides[22]},
    {gBlockSides[4],  gBlockSides[21]}, {gBlockSides[5],  gBlockSides[20]},
    {gBlockSides[6],  gBlockSides[19]}, {gBlockSides[7],  gBlockSides[18]},
    {gBlockSides[8],  gBlockSides[17]}, {gBlockSides[9],  gBlockSides[16]},
    {gBlockSides[10], gBlockSides[15]}, {gBlockSides[11], gBlockSides[14]},
    {gBlockSides[12], gBlockSides[13]}, {gBlockSides[13], gBlockSides[12]},
    {gBlockSides[14], gBlockSides[11]}, {gBlockSides[15], gBlockSides[10]},
    {gBlockSides[16], gBlockSides[ 9]}, {gBlockSides[17], gBlockSides[ 8]},
    {gBlockSides[18], gBlockSides[ 7]}, {gBlockSides[19], gBlockSides[ 6]},
    {gBlockSides[20], gBlockSides[ 5]}, {gBlockSides[21], gBlockSides[ 4]},
    {gBlockSides[22], gBlockSides[ 3]}, {gBlockSides[23], gBlockSides[ 2]},
    {gBlockSides[24], gBlockSides[ 1]}, {gBlockSides[25], gBlockSides[ 0]} };

inline blockSide_t oppositeSide(blockSide_t side)
{
  return gOppositeSides.find(side)->second;
}
inline blockSide_t getSide(const Point3i &dir)
{
  return ((dir[0] < 0 ? blockSide_t::NX : (dir[0] > 0 ? blockSide_t::PX : blockSide_t::NONE)) |
          (dir[1] < 0 ? blockSide_t::NY : (dir[1] > 0 ? blockSide_t::PY : blockSide_t::NONE)) |
          (dir[2] < 0 ? blockSide_t::NZ : (dir[2] > 0 ? blockSide_t::PZ : blockSide_t::NONE)) );
}
inline blockSide_t getSide(int sx, int sy, int sz)
{
  return ((sx < 0 ? blockSide_t::NX : (sx > 0 ? blockSide_t::PX : blockSide_t::NONE)) |
          (sy < 0 ? blockSide_t::NY : (sy > 0 ? blockSide_t::PY : blockSide_t::NONE)) |
          (sz < 0 ? blockSide_t::NZ : (sz > 0 ? blockSide_t::PZ : blockSide_t::NONE)) );
}
inline int sideIndex(blockSide_t side)
{ return gBlockSideIndices[side]; }

typedef uint32_t sideFlag_t;
static const uint32_t gAllSideFlags = (1 << gBlockSides.size()) - 1;
inline sideFlag_t sideFlag(blockSide_t side)
{ return (1 << gBlockSideIndices[side]); }
inline bool allSides(sideFlag_t flags)
{ return flags == gAllSideFlags; }

inline Point3i sideDirection(blockSide_t side)
{
  return gSideDirections[side];
}

inline normal_t sideToNormal(blockSide_t side)
{
  switch(side)
    {
    case blockSide_t::PX:
      return normal_t::PX;
    case blockSide_t::PY:
      return normal_t::PY;
    case blockSide_t::PZ:
      return normal_t::PZ;
    case blockSide_t::NX:
      return normal_t::NX;
    case blockSide_t::NY:
      return normal_t::NY;
    case blockSide_t::NZ:
      return normal_t::NZ;
    }
}
inline blockSide_t normalToSide(normal_t norm)
{
  return (blockSide_t)(1 << (int)norm);
}

inline int sideDim(blockSide_t side)
{
  switch(side)
    {
    case blockSide_t::PX:
    case blockSide_t::NX:
      return 0;
    case blockSide_t::PY:
    case blockSide_t::NY:
      return 1;
    case blockSide_t::PZ:
    case blockSide_t::NZ:
      return 2;
    }
}
inline int sideSign(blockSide_t side)
{
  switch(side)
    {
    case blockSide_t::PX:
    case blockSide_t::PY:
    case blockSide_t::PZ:
      return 1;
    case blockSide_t::NX:
    case blockSide_t::NY:
    case blockSide_t::NZ:
      return -1;
    }
}

#endif // BLOCK_SIDES_HPP
