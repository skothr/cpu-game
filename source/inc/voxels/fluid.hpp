#ifndef FLUID_HPP
#define FLUID_HPP

#include "block.hpp"
#include "logging.hpp"

#define MIN_FLUID_LEVEL 0.0001
static const std::array<float, BLOCK_FLUID_COUNT> gFluidEvap
  {{    0.0001f, // WATER
        0.0001f // LAVA
  }};

class FluidData : public BlockData
{
public:
  virtual int dataSize() const override { return sizeof(data); }
  
  FluidData(float evap, float level)
    : fluidEvap(evap), fluidLevel(level)
  { }
  FluidData(const uint8_t *dataIn)
  {
    deserialize(dataIn);
  }
  FluidData(const FluidData &other)
    : fluidEvap(other.fluidEvap), fluidLevel(other.fluidLevel)
  { }
  FluidData& operator=(const FluidData &other)
  {
    fluidEvap = other.fluidEvap;
    fluidLevel = other.fluidLevel;
    return *this;
  }

  virtual BlockData* copy() const override
  { return new FluidData(*this); }
  
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
    std::memcpy((void*)dataOut, (void*)data, dataSize());
    return dataSize();
  }
  int deserialize(const uint8_t *dataIn)
  {
    std::memcpy((void*)data, (void*)dataIn, sizeof(data));
  }

  bool gone() const
  { return fluidLevel <= MIN_FLUID_LEVEL; }
  
  union
  {
    struct
    {
      float fluidEvap;
      float fluidLevel;
    };
    uint8_t data[sizeof(float)*2];
  };
};


#endif // FLUID_HPP
