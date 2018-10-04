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
   PERLIN_CAVES,

   TEST,
   COUNT
  };


static const std::vector<std::string> gTerrainStrings = { "Dirt Ground",
                                                          "Perlin World",
                                                          "Perlin",
                                                          "Perlin Caves",
                                                          "TEST" };

inline std::string toString(terrain_t t)
{
  return (((int)t < (int)terrain_t::COUNT &&
           (int)t > (int)terrain_t::INVALID) ? gTerrainStrings[(int)t] : "<INVALID>" );
}
inline terrain_t terrainFromString(const std::string &str)
{
  auto iter = std::find(gTerrainStrings.begin(), gTerrainStrings.end(), str);
  if(iter != gTerrainStrings.end())
    { return (terrain_t)(iter - gTerrainStrings.begin()); }
  else
    { return terrain_t::INVALID; }
}

class TerrainGenerator
{
public:
  TerrainGenerator(uint32_t seed)
    : mSeed(seed)
  {
    mNoise.SetNoiseType(FastNoise::Simplex);
    mNoise.SetFrequency(1.0);
    mNoise.SetSeed(seed);
    //mNoise.SetInterp(FastNoise::Linear);
    mNoise.SetFractalOctaves(4);
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
