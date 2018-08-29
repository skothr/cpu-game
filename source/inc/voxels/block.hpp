#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <vector>
#include <cstdint>
#include <array>
#include <unordered_map>

#include "model.hpp"
#include "helpers.hpp"
#include "vector.hpp"
#include "blockSides.hpp"

class cShader;

enum class simple_t : uint8_t
  {
   NONE = 0,
   
   START = 1,
   DIRT = START,
   GRASS,
   STONE,
   SAND,

   END
  };
enum class complex_t : uint8_t
  {
   NONE = 0,

   START = (int)simple_t::END,
   DEVICE = START,
   CPU,
   MEMORY,
   LIGHT,
   
   END
  };
enum class fluid_t : uint8_t
  {
   NONE = 0,
   
   START =  (int)complex_t::END,
   WATER = START,

   END
  };

enum class block_t : uint8_t
  {
   NONE = 0,
   // simple blocks
   DIRT = (int)simple_t::START,
   GRASS,
   STONE,
   SAND,
   // complex blocks
   DEVICE = (int)complex_t::START,
   CPU,
   MEMORY,
   LIGHT,
   // fluid blocks
   WATER = (int)fluid_t::START,
   
   COUNT
  };
#define BLOCK_SIMPLE_COUNT  ((int)simple_t::END  - (int)simple_t::START)
#define BLOCK_COMPLEX_COUNT ((int)complex_t::END - (int)complex_t::START)
#define BLOCK_FLUID_COUNT   ((int)fluid_t::END   - (int)fluid_t::START)

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
    case block_t::DEVICE:
      return "DEVICE";
    case block_t::CPU:
      return "CPU";
    case block_t::MEMORY:
      return "MEMORY";
    case block_t::LIGHT:
      return "LIGHT";
    case block_t::WATER:
      return "WATER";
    default:
      return "<UNKNOWN BLOCK TYPE>";
    }
}

inline bool isSimpleBlock(block_t type)
{
  return (((int)type >= (int)simple_t::START) &&
	  ((int)type < (int)simple_t::END) );
}
inline int simpleIndex(block_t type)
{
  return (int)type - (int)simple_t::START;
}
inline block_t simpleType(int index)
{
  return ((index >= (int)simple_t::START) && index < (int)simple_t::END ?
          (block_t)(index + (int)simple_t::START) : block_t::NONE );
}

inline bool isComplexBlock(block_t type)
{
  return (((int)type >= (int)complex_t::START) &&
	  ((int)type < (int)complex_t::END) );
}
inline int complexIndex(block_t type)
{
  return (int)type - (int)complex_t::START;
}
inline block_t complexType(int index)
{
  return ((index >= (int)complex_t::START) && index < (int)complex_t::END ?
          (block_t)(index + (int)complex_t::START) : block_t::NONE );
}

inline bool isFluidBlock(block_t type)
{
  return (((int)type >= (int)fluid_t::START) &&
	  ((int)type < (int)fluid_t::END) );
}
inline int fluidIndex(block_t type)
{
  return (int)type - (int)fluid_t::START;
}
inline block_t fluidType(int index)
{
  return ((index >= (int)fluid_t::START) && index < (int)fluid_t::END ?
          (block_t)(index + (int)fluid_t::START) : block_t::NONE );
}


#define ATLAS_SIZE 512
#define ATLAS_BLOCK_SIZE 64
#define ATLAS_W (ATLAS_SIZE / ATLAS_BLOCK_SIZE)

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
