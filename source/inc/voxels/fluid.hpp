#ifndef FLUID_HPP
#define FLUID_HPP

#include "block.hpp"
#include "logging.hpp"

#define MIN_FLUID_LEVEL 0.0001f
static std::unordered_map<block_t, float> gFluidEvap
  { {block_t::WATER, 0.0001f},
    {block_t::LAVA, 0.0001f} };

class Fluid : public BlockData
{
public:
  
  Fluid(block_t type, float evap, float level)
    : type(type), fluidEvap(evap), fluidLevel(level)
  { }
  Fluid(const uint8_t *dataIn)
  {
    deserialize(dataIn);
  }
  Fluid(const Fluid &other)
    : type(other.type), fluidEvap(other.fluidEvap), fluidLevel(other.fluidLevel)
  { }
  Fluid& operator=(const Fluid &other)
  {
    type = other.type;
    fluidEvap = other.fluidEvap;
    fluidLevel = other.fluidLevel;
    return *this;
  }

  virtual BlockData* copy() const override
  { return new Fluid(type, fluidEvap, fluidLevel); }
  
  bool step()
  {
    fluidLevel -= fluidEvap;
    return true;
  }

  float adjustLevel(float levelChange)
  {
    fluidLevel += levelChange;
    float overflow = 0.0f;
    if(fluidLevel < 0.0f)
      {
        overflow = fluidLevel;
        fluidLevel = 0.0f;
      }
    else if(fluidLevel > 1.0f)
      {
        overflow = fluidLevel - 1.0f;
        fluidLevel = 1.0f;
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
  { return fluidLevel <= MIN_FLUID_LEVEL; }
  
  union
  {
    struct
    {
      block_t type;
      float fluidEvap;
      float fluidLevel;
    };
    uint8_t data[sizeof(float)*2];
  };
  static const int dataSize = sizeof(data);
};


#endif // FLUID_HPP
