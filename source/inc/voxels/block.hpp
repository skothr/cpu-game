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

class Shader;

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
enum class fluid_t : uint8_t
  {
   NONE = 0,
   
   START = (int)simple_t::END,
   WATER = START,
   LAVA,

   END
  };
enum class complex_t : uint8_t
  {
   NONE = 0,

   START = (int)fluid_t::END,
   DEVICE = START,
   CPU,
   MEMORY,
   LIGHT,
   
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
   // fluid blocks
   WATER = (int)fluid_t::START,
   LAVA,
   // complex blocks
   DEVICE = (int)complex_t::START,
   CPU,
   MEMORY,
   LIGHT,
   
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
    case block_t::LAVA:
      return "LAVA";
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

namespace Block
{
  static const int dataSize = sizeof(block_t);
};

class BlockData // base class for block data (e.g. fluid data)
{
public:
  //virtual int dataSize() const = 0;
  virtual BlockData* copy() const = 0;
};


struct CompleteBlock
{
  block_t type = block_t::NONE;
  BlockData *data = nullptr;
};


class ComplexBlock : public BlockData
{
public:
  ComplexBlock() {}
  virtual ~ComplexBlock() {}

  virtual block_t type() const = 0;
  virtual void update() {}
  virtual void makeConnection(const CompleteBlock &other) {}
};


class Device;
class DeviceBlock : public ComplexBlock
{
public:
  DeviceBlock();
  virtual ~DeviceBlock();

  virtual block_t type() const { return block_t::DEVICE; }
  virtual BlockData* copy() const;
  virtual void update();
  virtual void makeConnection(const CompleteBlock &other);
  
private:
  Device *mDevice;
};

class Cpu;
class CpuBlock : public ComplexBlock
{
public:
  CpuBlock(int numBits, int numRegs, int cpuSpeed);
  virtual ~CpuBlock();
  Cpu* getCpu() { return mCpu; }

  virtual block_t type() const { return block_t::CPU; }
  virtual BlockData* copy() const;
  virtual void update();
  virtual void makeConnection(const CompleteBlock &other);
  
private:
  Cpu *mCpu;
};

class Memory;
class MemoryBlock : public ComplexBlock
{
public:
  MemoryBlock(int numBytes, int memSpeed);
  virtual ~MemoryBlock();

  Memory* getMemory() { return mMemory; }

  virtual block_t type() const { return block_t::MEMORY; }
  virtual BlockData* copy() const;
  virtual void update();
  virtual void makeConnection(const CompleteBlock &other);
  
private:
  Memory *mMemory;
};


#endif // BLOCK_HPP
