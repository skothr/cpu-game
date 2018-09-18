#ifndef FLUID_HPP
#define FLUID_HPP

#include "block.hpp"
#include "logging.hpp"
#include "vector.hpp"


#define MIN_FLUID_LEVEL 0.001f
static std::unordered_map<block_t, float> gFluidEvap
  { {block_t::WATER, 0.001f},
    {block_t::LAVA, 0.001f} };

class Fluid : public BlockData
{
public:
  Fluid(block_t type=block_t::WATER, float level=0.0f)
    : type(type), level(level), nextLevel(level)
  {
    //std::memset(sides, 0, sizeof(float)*4);
  }
  Fluid(const uint8_t *dataIn)
  {
    deserialize(dataIn);
  }
  Fluid(const Fluid &other)
    : type(other.type), level(other.level), sideLevel(other.sideLevel),
      falling(other.falling), nextLevel(other.nextLevel), nextSideLevel(other.nextSideLevel)
  { }
  Fluid& operator=(const Fluid &other)
  {
    type = other.type;
    level = other.level;
    sideLevel = other.sideLevel;
    nextLevel = other.nextLevel;
    nextSideLevel = other.nextSideLevel;
    falling = other.falling;
    return *this;
  }

  virtual BlockData* copy() const override
  {
    Fluid *f = new Fluid(type, level);
    f->sideLevel = sideLevel;
    f->nextLevel = nextLevel;
    f->nextSideLevel = nextSideLevel;
    f->falling = falling;
    return f;
  }
   
  
  bool step(float evap)
  {
    level -= evap;
    return true;
  }

  float adjustLevel(float levelChange)
  {
    level += levelChange;
    float overflow = 0.0f;
    if(level < 0.0f)
      {
        overflow = level;
        level = 0.0f;
      }
    else if(level > 1.0f)
      {
        overflow = level - 1.0f;
        level = 1.0f;
      }
    return overflow;
  }
  float adjustSideLevel(int side, float levelChange)
  {
    sideLevel[side] += levelChange;
    float overflow = 0.0f;
    if(sideLevel[side] < 0.0f)
      {
        overflow = sideLevel[side];
        sideLevel[side] = 0.0f;
      }
    else if(sideLevel[side] > 1.0f)
      {
        overflow = sideLevel[side] - 1.0f;
        sideLevel[side] = 1.0f;
      }
    return overflow;
  }
  float adjustNextLevel(float levelChange)
  {
    nextLevel += levelChange;
    float overflow = 0.0f;
    if(nextLevel < 0.0f)
      {
        overflow = nextLevel;
        nextLevel = 0.0f;
      }
    else if(nextLevel > 1.0f)
      {
        overflow = nextLevel - 1.0f;
        nextLevel = 1.0f;
      }
    return overflow;
  }
  float adjustNextSideLevel(int side, float levelChange)
  {
    nextSideLevel[side] += levelChange;
    float overflow = 0.0f;
    if(nextSideLevel[side] < 0.0f)
      {
        overflow = nextSideLevel[side];
        nextSideLevel[side] = 0.0f;
      }
    else if(nextSideLevel[side] > 1.0f)
      {
        overflow = nextSideLevel[side] - 1.0f;
        nextSideLevel[side] = 1.0f;
      }
    return overflow;
  }
  int serialize(uint8_t *dataOut) const
  {
    std::memcpy((void*)dataOut, (void*)data, dataSize);
    return dataSize;
  }
  int deserialize(const uint8_t *dataIn)
  {
    std::memcpy((void*)data, (void*)dataIn, sizeof(data));
  }

  bool isEmpty() const
  {
    return (level <= MIN_FLUID_LEVEL &&
            sideLevel[0] <= MIN_FLUID_LEVEL && sideLevel[1] <= MIN_FLUID_LEVEL &&
            sideLevel[2] <= MIN_FLUID_LEVEL && sideLevel[3] <= MIN_FLUID_LEVEL );
  }
  bool nextEmpty() const
  {
    return (nextLevel <= MIN_FLUID_LEVEL &&
            nextSideLevel[0] <= MIN_FLUID_LEVEL && nextSideLevel[1] <= MIN_FLUID_LEVEL &&
            nextSideLevel[2] <= MIN_FLUID_LEVEL && nextSideLevel[3] <= MIN_FLUID_LEVEL );
  }

  bool falling = false;
  Vector<float, 4> sideLevel;
  
  Vector<float, 4> nextSideLevel;
  float nextLevel;
      
  union
  {
    struct
    {
      block_t type;
      float level;
    };
    uint8_t data[sizeof(float)*2];
  };
  static const int dataSize = sizeof(data);
};


#endif // FLUID_HPP
