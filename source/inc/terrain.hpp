#ifndef TERRAIN_HPP
#define TERRAIN_HPP


#include <cmath>
#include <vector>
#include <string>
#include <ios>

#include "vector.hpp"

enum class terrain_t : uint8_t
  {
   INVALID = 0,
   DIRT_GROUND,
   PERLIN,
   PERLIN_CHUNK
  };

class cPerlinNoise
{
public:
  cPerlinNoise(uint32_t seed = 0);
  ~cPerlinNoise();


  void setSeed(uint32_t seed);

  inline double noise(double x, double y, double z)
  {
    int x2 = (int)std::floor(x) & 255;
    int y2 = (int)std::floor(y) & 255;
    int z2 = (int)std::floor(z) & 255;
    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);
    double u = fade(x), v = fade(y), w = fade(z);
    int a = mVals[x2] + y2, aa = mVals[a] + z2, ab = mVals[a+1] + z2,
      b = mVals[x2+1]+y2, ba = mVals[b] + z2, bb = mVals[b+1]+z2;
    
    return lerp(w, lerp(v, lerp(u, grad(mVals[aa  ], x, y, z),
				grad(mVals[ba], x-1, y, z) ),
			lerp(u, grad(mVals[ab], x, y-1, z), 
			     grad(mVals[bb], x-1, y-1, z) )),
		lerp(v, lerp(u, grad(mVals[aa+1], x, y, z-1),
			     grad(mVals[ba+1], x-1, y, z-1) ),
		     lerp(u, grad(mVals[ab+1], x, y-1, z-1),
			  grad(mVals[bb+1], x-1, y-1, z-1) )));
  }

    
private:
  std::vector<int> mVals;
  
  inline double fade(double t)
  { return t * t * t * (t * (t * 6 - 0x0F) + 10); }
  inline double lerp(double t, double a, double b)
  { return a + t * (b - a); }
  inline double grad(int hash, double x, double y, double z)
  {
    int h = hash & 0x0F;
    double u = (h < 8 ? x : y);
    double v = (h < 4 ? y : (h == 12 || h == 14 ? x : z));
    return ((h & 0x01) == 0 ? u : -u) + ((h & 0x02) == 0 ? v : -v);
  }
  
  // inline int perlinNoise(int x, int y, float scale, float mag, float exp)
  // {
  //   return (int)(std::pow(()));
  // }
};



class cTerrainGenerator
{
public:
  cTerrainGenerator(uint32_t seed)
    : mSeed(seed), mNoise(seed)
  { }

  void setSeed(uint32_t seed)
  {
    mSeed = seed;
    mNoise.setSeed(mSeed);
  }
  uint32_t getSeed() const { return mSeed; }
  
  void generate(const Point3i &chunkPos, terrain_t genType,
                     std::vector<uint8_t> &dataOut);
private:
  cPerlinNoise mNoise;
  uint32_t mSeed;
};

  
#endif // TERRAIN_HPP
