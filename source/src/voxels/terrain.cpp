#include "terrain.hpp"
#include "chunk.hpp"
#include <random>
#include <numeric>
#include <algorithm>

PerlinNoise::PerlinNoise(unsigned int seed)
{
  setSeed(seed);
}

PerlinNoise::~PerlinNoise()
{
  
}

void PerlinNoise::setSeed(uint32_t seed)
{
  mVals.resize(256);
  // fill vals with [0, 255]
  std::iota(mVals.begin(), mVals.end(), 0);
  // init random engine with given seed
  std::default_random_engine engine(seed);
  // shuffle using the random engine
  std::shuffle(mVals.begin(), mVals.end(), engine);
  // duplicate permutation vector
  mVals.insert(mVals.end(), mVals.begin(), mVals.end());
}








void TerrainGenerator::generate(const Point3i &chunkPos, terrain_t genType,
                                std::vector<uint8_t> &dataOut )
{
  dataOut.resize(Chunk::totalSize * Block::dataSize);
  Block b;
  int x,y,z;
  switch(genType)
    {
    case terrain_t::DIRT_GROUND:
      for(x = 0; x < Chunk::sizeX; x++)
        for(y = 0; y < Chunk::sizeY; y++)
          for(z = 0; z < Chunk::sizeZ; z++)
            {
              int i = mIndexer.index(x, y, z);
              
              if(chunkPos[2]*Chunk::sizeZ + z < 4)
                b.type =  block_t::DIRT;
              else
                b.type = block_t::NONE;

              b.serialize(&dataOut[i*Block::dataSize]);
              //std::memcpy((void*), (void*)&b.data, Block::dataSize);
            }
      break;              
    case terrain_t::PERLIN:
      for(x = 0; x < Chunk::sizeX; x++)
        for(y = 0; y < Chunk::sizeY; y++)
          for(z = 0; z < Chunk::sizeZ; z++)
            {
              int i = mIndexer.index(x, y, z);

              Point3i worldPos = Point3i{chunkPos[0]*Chunk::sizeX+x, chunkPos[1]*Chunk::sizeY + y, chunkPos[2]*Chunk::sizeZ + z}*4;
              worldPos[2]*=3;
              double n0 = mNoise.noise((double)worldPos[0]/Chunk::sizeX, (double)worldPos[1]/Chunk::sizeY, (double)worldPos[2]/Chunk::sizeZ);
              double n1 = mNoise.noise((double)worldPos[0]/Chunk::sizeX/8.0, (double)worldPos[1]/Chunk::sizeY/8.0, (double)worldPos[2]/Chunk::sizeZ/8.0);
              double n2 = mNoise.noise((double)worldPos[0]/Chunk::sizeX/16.0, (double)worldPos[1]/Chunk::sizeY/16.0, (double)worldPos[2]/Chunk::sizeZ/8.0);


              double n = 100*n0 - 3.0*(worldPos[2]) + 1000*std::abs(n2);
              double nn = 10*n1;
              
              if(n < 0)
                { b.type = block_t::NONE; }
              else if(n < 75.0)
                { b.type = block_t::GRASS; }
              else if(n < 150.0)
                {
                  b.type = (nn > 0 ? block_t::DIRT : block_t::SAND);
                }
              else
                { b.type = block_t::STONE; }

              //if(b.type != block_t::NONE)
              ///{ LOGD("GENERATED BLOCK --> %d", (int)b.type); }
              b.serialize(&dataOut[i*Block::dataSize]);
              //std::memcpy((void*)&dataOut[i*Block::dataSize], (void*)&b.data, Block::dataSize);
            }
      break;
    case terrain_t::PERLIN_CHUNK:
      for(x = 0; x < Chunk::sizeX; x++)
        for(y = 0; y < Chunk::sizeY; y++)
          for(z = 0; z < Chunk::sizeZ; z++)
            {
              int i = mIndexer.index(x, y, z);
              Point3i worldPos = Point3i{chunkPos[0]*Chunk::sizeX+x, chunkPos[1]*Chunk::sizeY+y, chunkPos[2]*Chunk::sizeZ+z};
              double n0 = mNoise.noise((double)worldPos[0]/Chunk::sizeX, (double)worldPos[1]/Chunk::sizeY, (double)worldPos[2]/Chunk::sizeZ);
              double n1 = mNoise.noise((double)worldPos[0]/Chunk::sizeX/8.0, (double)worldPos[1]/Chunk::sizeY/8.0, (double)worldPos[2]/Chunk::sizeZ/8.0);

              double n = 1000*n0 - (worldPos[2]);
              
              if(n < 0)
                { b.type = block_t::NONE; }
              else if(n < 75.0)
                { b.type = ((n1 > 0 || n1 < -100.0) ? block_t::GRASS : block_t::SAND); }
              else if(n < 150.0)
                { b.type = block_t::DIRT; }
              else
                { b.type = block_t::STONE; }

              //if(b.type != block_t::NONE)
              //{ LOGD("GENERATED BLOCK --> %d", (int)b.type); }
              
              b.serialize(&dataOut[i*Block::dataSize]);
              //std::memcpy((void*)&dataOut[i*Block::dataSize], (void*)&b.data, Block::dataSize);
            }
      break;
    }
}
