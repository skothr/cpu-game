#include "terrain.hpp"

#include <random>
#include <numeric>
#include <algorithm>

cPerlinNoise::cPerlinNoise(unsigned int seed)
  : mSeed(seed)
{
  setSeed(seed);
}

cPerlinNoise::~cPerlinNoise()
{
  
}

void cPerlinNoise::setSeed(uint32_t seed)
{
  mSeed = seed;
  mVals.resize(256);
  // fill vals with [0, 255]
  std::iota(mVals.begin(), mVals.end(), 0);

  // init random engine with given seed
  std::default_random_engine engine(mSeed);
  // shuffle using the random engine
  std::shuffle(mVals.begin(), mVals.end(), engine);
  // duplicate permutation vector
  mVals.insert(mVals.end(), mVals.begin(), mVals.end());
}
