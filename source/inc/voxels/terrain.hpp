#ifndef TERRAIN_HPP
#define TERRAIN_HPP


#include <cmath>
#include <vector>
#include <string>
#include <ios>
#include "FastNoise.h"

#include "vector.hpp"
#include "chunk.hpp"
#include "indexing.hpp"

enum class terrain_t : int8_t
  {
   INVALID = -1,
   DIRT_GROUND = 0,
   PERLIN_WORLD,
   PERLIN,

   COUNT
  };

inline std::string toString(terrain_t t)
{
  switch(t)
    {
    case terrain_t::DIRT_GROUND:
      return "Dirt Ground";
    case terrain_t::PERLIN:
      return "Perlin";
    case terrain_t::PERLIN_WORLD:
      return "Perlin World";
    default:
      return "<INVALID>";
    }
}
inline terrain_t terrainFromString(const std::string &str)
{
  if(str == "Dirt Ground")
    {
      return terrain_t::DIRT_GROUND;
    }
  else if(str == "Perlin")
    {
      return terrain_t::PERLIN;
    }
  else if(str == "Perlin World")
    {
      return terrain_t::PERLIN_WORLD;
    }
  else
    { return terrain_t::INVALID; }
}

class TerrainGenerator
{
public:
  TerrainGenerator(uint32_t seed)
    : mSeed(seed)
  {
    mNoise.SetNoiseType(FastNoise::SimplexFractal);
    mNoise.SetFrequency(1.0);
    mNoise.SetSeed(seed);
    mNoise.SetInterp(FastNoise::Linear);
    mNoise.SetFractalOctaves(1);
  }

  void setSeed(uint32_t seed)
  {
    mSeed = seed;
    mNoise.SetSeed(mSeed);
  }
  uint32_t getSeed() const { return mSeed; }
  
  void generate(const Point3i &chunkPos, terrain_t genType,
                     std::vector<uint8_t> &dataOut);
private:
  FastNoise mNoise;
  uint32_t mSeed;
  Indexer<Chunk::sizeX, Chunk::sizeY, Chunk::sizeZ> mIndexer;
};

  
#endif // TERRAIN_HPP
