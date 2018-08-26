#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <vector>
#include <cstdint>
#include <array>
#include <unordered_map>

#include "model.hpp"
#include "helpers.hpp"
#include "vector.hpp"

class cShader;

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


static const std::vector<blockSide_t> gBlockSides =
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

// directions from center
static const std::unordered_map<blockSide_t, Point3i> gSideDirections =
  { {gBlockSides[0],  Point3i({-1,-1,-1})},
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
static const std::unordered_map<blockSide_t, blockSide_t> gOppositeSides =
    { {gBlockSides[0],  gBlockSides[25]}, {gBlockSides[1],  gBlockSides[24]},
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
  return (side == blockSide_t::NONE ? blockSide_t::NONE :
          gOppositeSides.find(side)->second );
}
inline blockSide_t getSide(int sx, int sy, int sz)
{
  return ((sx < 0 ? blockSide_t::NX : (sx > 0 ? blockSide_t::PX : blockSide_t::NONE)) |
          (sy < 0 ? blockSide_t::NY : (sy > 0 ? blockSide_t::PY : blockSide_t::NONE)) |
          (sz < 0 ? blockSide_t::NZ : (sz > 0 ? blockSide_t::PZ : blockSide_t::NONE)) );
}

inline Point3i sideDirection(blockSide_t side)
{
  return gSideDirections.find(side)->second;
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

enum class block_t : uint8_t
  {
   NONE = 0,
   SIMPLE_START,
   DIRT = SIMPLE_START,
   GRASS,
   STONE,
   SAND,

   COMPLEX_START,
   DEVICE = COMPLEX_START,
   CPU,
   MEMORY,
   LIGHT,

   FLUID_START,
   WATER = FLUID_START,
   
   COUNT
  };
#define BLOCK_SIMPLE_COUNT ((int)block_t::COMPLEX_START - (int)block_t::SIMPLE_START)
#define BLOCK_COMPLEX_COUNT ((int)block_t::FLUID_START - (int)block_t::COMPLEX_START)

inline std::string toString(block_t b)
{
  switch(b)
    {
    case block_t::NONE:
      return "NONE";
    case block_t::DIRT:
      return "DIRT";
    case block_t::GRASS:
      return "GRASS";
    case block_t::STONE:
      return "STONE";
    case block_t::SAND:
      return "SAND";
    case block_t::WATER:
      return "WATER";
    case block_t::DEVICE:
      return "DEVICE";
    case block_t::CPU:
      return "CPU";
    case block_t::MEMORY:
      return "MEMORY";
    case block_t::LIGHT:
      return "LIGHT";
    default:
      return "<UNKNOWN BLOCK TYPE>";
    }
}

inline bool isSimpleBlock(block_t type)
{
  switch(type)
    {
    case block_t::DIRT:
    case block_t::GRASS:
    case block_t::STONE:
    case block_t::SAND:
      return true;
    default:
      return false;
    }
  /*
  return (((int)type > (int)block_t::NONE) &&
	  ((int)type < (int)block_t::SIMPLE_COUNT) );
  */
}
inline int simpleIndex(block_t type)
{
  return (int)type - 1;
}
inline block_t simpleType(int index)
{
  return (index >= 0 && index < BLOCK_SIMPLE_COUNT ?
	  (block_t)(index + 1) :
    	  block_t::NONE );
}

inline bool isComplexBlock(block_t type)
{
  switch(type)
    {
    case block_t::DEVICE:
    case block_t::CPU:
    case block_t::MEMORY:
    case block_t::LIGHT:
      return true;
    default:
      return false;
    }
  /*
  return (((int)type >= (int)block_t::SIMPLE_COUNT) &&
	  ((int)type < (int)block_t::COUNT) );
  */
}
inline int complexIndex(block_t type)
{
  return (int)type - (int)block_t::COMPLEX_START;
}
inline block_t complexType(int index)
{
  return (index >= 0 && index < BLOCK_COMPLEX_COUNT ?
	  (block_t)((int)block_t::COMPLEX_START + index) :
	  block_t::NONE );
}


#define ATLAS_SIZE 512
#define ATLAS_BLOCK_SIZE 64
#define ATLAS_W (ATLAS_SIZE / ATLAS_BLOCK_SIZE)

/*
static constexpr float BLOCK_ATLAS_MULT = 1.0f / (float)ATLAS_W - 2.0f / ATLAS_SIZE;
inline Point2f blockAtlasOffset(block_t type)
{
  const int index = (int)type - 1;
  return Point2f({(float)(index % ATLAS_W) / (float)ATLAS_W + 1.0f/ATLAS_SIZE,
		  (float)(index / ATLAS_W) / (float)ATLAS_W + 1.0f/ATLAS_SIZE });
}
*/

#define MAX_LIGHT_LEVEL 16
#define DEFAULT_LIGHT_LEVEL 0
#define BLOCK_USED_BITS 22

class cBlock
{
public:
  cBlock(block_t t = block_t::NONE, blockSide_t sides = blockSide_t::ALL,
	 int8_t light = DEFAULT_LIGHT_LEVEL )
    : type(t), activeSides(sides), lightLevel(light), occlusion(type == block_t::NONE ? 0 : 1)
  { }

  static const int dataSize = 1; // data bytes to store
  
  union
  {
    struct
    {
      block_t type;
      // union
      // {
      // 	uint8_t lightLevel    : 8; // if block is occupied
      // 	uint8_t airPressure   : 8; // if block is not occupied
      // };
    };
    uint32_t data; //TOTAL
  };
  
  blockSide_t activeSides; // sides of block that are showing
  int8_t lightLevel;
  uint8_t occlusion;
  
  virtual void update() { }

  inline void setActive(blockSide_t side, block_t adjType)
  {
    activeSides = (type == block_t::NONE ? blockSide_t::NONE :
                   (adjType == block_t::NONE ? (activeSides | side) : (activeSides & ~side)) );
  }
  inline void setActive(blockSide_t side)
  { activeSides |= side; }
  inline void setInactive(blockSide_t side)
  { activeSides &= ~side; }


  void setLighting(int8_t lighting)
  {
    lightLevel = lighting;
    /*
    if(lighting < 0)
      { lightLevel = 0; }
    else if(lighting > MAX_LIGHT_LEVEL)
      { lightLevel = MAX_LIGHT_LEVEL; }
    else
      { lightLevel = lighting; }
    */
  }
  void updateOcclusion();
  
  bool active() const;
  bool active(blockSide_t side) const;

  bool activeEmpty(blockSide_t side);
  bool activeMirror(blockSide_t side);
  
  // modifies pointer and returns leftover bits in pointed-to byte
  virtual int serialize(uint8_t *dataOut) const;
  virtual void deserialize(const uint8_t *dataIn, int bytes);
};

/*
class cComplexBlock : public cBlock
{
public:
  cComplexBlock(block_t t = block_t::NONE, blockSide_t sides = blockSide_t::ALL,
	 uint8_t light = DEFAULT_LIGHT_LEVEL )
    : type(t), activeSides(sides), lightLevel(light)
  { }
};
*/





#endif // BLOCK_HPP
