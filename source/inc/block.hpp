#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <vector>
#include <cstdint>
#include "model.hpp"
#include "helpers.hpp"
#include <array>

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
inline blockSide_t oppositeSide(blockSide_t side)
{
  blockSide_t oSide = blockSide_t::NONE;
  oSide |= ((bool)(side & blockSide_t::PX) ? blockSide_t::NX : ((bool)(side & blockSide_t::NX) ? blockSide_t::PX : blockSide_t::NONE));
  oSide |= ((bool)(side & blockSide_t::PY) ? blockSide_t::NY : ((bool)(side & blockSide_t::NY) ? blockSide_t::PY : blockSide_t::NONE));
  oSide |= ((bool)(side & blockSide_t::PZ) ? blockSide_t::NZ : ((bool)(side & blockSide_t::NZ) ? blockSide_t::PZ : blockSide_t::NONE));

  return oSide;
}
inline blockSide_t getSide(int sx, int sy, int sz)
{
  blockSide_t oSide = blockSide_t::NONE;
  oSide |= (sx < 0 ? blockSide_t::NX : (sx > 0 ? blockSide_t::PX : blockSide_t::NONE));
  oSide |= (sy < 0 ? blockSide_t::NY : (sy > 0 ? blockSide_t::PY : blockSide_t::NONE));
  oSide |= (sz < 0 ? blockSide_t::NZ : (sz > 0 ? blockSide_t::PZ : blockSide_t::NONE));

  return oSide;
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
   DIRT,
   GRASS,
   STONE,
   SAND,
   
   SIMPLE_COUNT,

   WATER = SIMPLE_COUNT,
   DEVICE,
   CPU,
   MEMORY,
   LIGHT,
   
   COUNT
  };
#define BLOCK_SIMPLE_COUNT ((int)block_t::SIMPLE_COUNT - 1)
#define BLOCK_COMPLEX_COUNT ((int)block_t::COUNT - (int)block_t::SIMPLE_COUNT)

inline bool isSimpleBlock(block_t type)
{
  return (((int)type > (int)block_t::NONE) &&
	  ((int)type < (int)block_t::SIMPLE_COUNT) );
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
  return (((int)type >= (int)block_t::SIMPLE_COUNT) &&
	  ((int)type < (int)block_t::COUNT) );
}
inline int complexIndex(block_t type)
{
  return (int)type - (int)block_t::SIMPLE_COUNT;
}
inline block_t complexType(int index)
{
  return (index >= 0 && index < BLOCK_COMPLEX_COUNT ?
	  (block_t)((int)block_t::SIMPLE_COUNT + index) :
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

#define MAX_LIGHT_LEVEL 3
#define DEFAULT_LIGHT_LEVEL 1
#define BLOCK_USED_BITS 22

class cBlock
{
public:
  cBlock(block_t t = block_t::NONE, blockSide_t sides = blockSide_t::ALL,
	 uint8_t light = DEFAULT_LIGHT_LEVEL )
    : type(t), activeSides(sides), lightLevel(light)
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
    uint8_t data; //TOTAL
  };
  blockSide_t activeSides; // sides of block that are showing
  uint8_t lightLevel;
  //blockSide_t dirtySides; // sides of block that need an update

  virtual void update() { }

  inline void setActive(blockSide_t side, block_t adjType)
  {
    activeSides = (adjType == block_t::NONE ?
                   (activeSides | side) : (activeSides & ~side) );
  }
  inline void setActive(blockSide_t side)
  { activeSides |= side; }
  inline void setInactive(blockSide_t side)
  { activeSides &= ~side; }
  
  bool active() const;
  bool active(blockSide_t side) const;
  //bool dirty() const;

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
