#include "terrain.hpp"
#include "chunk.hpp"
#include <random>
#include <numeric>
#include <algorithm>


// NOTE: For performance, check out https://github.com/Auburns/FastNoiseSIMD
void TerrainGenerator::generate(const Point3i &chunkPos, terrain_t genType,
                                std::vector<uint8_t> &dataOut )
{
  dataOut.resize(Chunk::totalSize * Block::dataSize);
  block_t b;
  int x,y,z;
  int bi = 0;
  switch(genType)
    {
    case terrain_t::DIRT_GROUND:
      for(x = 0; x < Chunk::sizeX; x++)
        for(y = 0; y < Chunk::sizeY; y++)
          for(z = 0; z < Chunk::sizeZ; z++)
            {
              int i = mIndexer.index(x, y, z);
              b = (chunkPos[2]*Chunk::sizeZ + z < 4) ? block_t::DIRT : block_t::NONE;

              //b.serialize(&dataOut[i*Block::dataSize]);
              std::memcpy((void*)&dataOut[i*Block::dataSize], (void*)&b, Block::dataSize);
            }
      break;              
    case terrain_t::PERLIN_WORLD:
      mNoise.SetFrequency(1.0);
      mNoise.SetNoiseType(FastNoise::Simplex);
      
      for(y = 0; y < Chunk::sizeY; y++)
        for(z = 0; z < Chunk::sizeZ; z++)
          for(x = 0; x < Chunk::sizeX; x++, bi++)
            {
              Point3i worldPos = Point3i{chunkPos[0]*Chunk::sizeX+x, chunkPos[1]*Chunk::sizeY + y, chunkPos[2]*Chunk::sizeZ + z}*4;
              //if(worldPos[2] > 64)
              //{ continue; }
              worldPos[2]*=3;
              float n0 = mNoise.GetNoise((float)worldPos[0]/Chunk::sizeX, (float)worldPos[1]/Chunk::sizeY, (float)worldPos[2]/Chunk::sizeZ);
              float n1 = mNoise.GetNoise((float)worldPos[0]/Chunk::sizeX/8.0, (float)worldPos[1]/Chunk::sizeY/8.0, (float)worldPos[2]/Chunk::sizeZ/8.0);
              float n2 = mNoise.GetNoise((float)worldPos[0]/Chunk::sizeX/16.0, (float)worldPos[1]/Chunk::sizeY/16.0, (float)worldPos[2]/Chunk::sizeZ/8.0);

              float n = 100*n0 - 3.0*(worldPos[2]) - 1000*std::abs(n2)*(0.5+n1);
              float nn = 10*n1;
              
              if(n < 0)
                { b = block_t::NONE; }
              else if(n < 75.0)
                { b = block_t::GRASS; }
              else if(n < 150.0)
                {
                  b = (nn > 0 ? block_t::DIRT : block_t::SAND);
                }
              else
                { b = block_t::STONE; }

              //if(b.type != block_t::NONE)
              ///{ LOGD("GENERATED BLOCK --> %d", (int)b.type); }
              
              std::memcpy((void*)&dataOut[bi*Block::dataSize], (void*)&b, Block::dataSize);
            }
      break;
    case terrain_t::PERLIN:
      mNoise.SetFrequency(1.0);
      mNoise.SetNoiseType(FastNoise::Simplex);
      for(y = 0; y < Chunk::sizeY; y++)
        for(z = 0; z < Chunk::sizeZ; z++)
          for(x = 0; x < Chunk::sizeX; x++, bi++)
            {
              Point3i worldPos = Point3i{chunkPos[0]*Chunk::sizeX+x, chunkPos[1]*Chunk::sizeY+y, chunkPos[2]*Chunk::sizeZ+z};
              float n0 = mNoise.GetNoise((float)worldPos[0]/Chunk::sizeX, (float)worldPos[1]/Chunk::sizeY, (float)worldPos[2]/Chunk::sizeZ);
              //mNoise.SetFrequency(1.0);
              float n1 = mNoise.GetNoise((float)worldPos[0]/Chunk::sizeX/8.0, (float)worldPos[1]/Chunk::sizeY/8.0, (float)worldPos[2]/Chunk::sizeZ/8.0);

              float n = 1000*n0 - worldPos[2];
              
              if(n < 0)
                { b = block_t::NONE; }
              else if(n < 75.0)
                //  { b = block_t::GRASS; }
              { b = ((n1 > 0 || n1 < -100.0) ? block_t::GRASS : block_t::SAND); }
              else if(n < 150.0)
                { b = block_t::DIRT; }
              else
                { b = block_t::STONE; }

              //if(b.type != block_t::NONE)
              //{ LOGD("GENERATED BLOCK --> %d", (int)b.type); }
              
              std::memcpy((void*)&dataOut[bi*Block::dataSize], (void*)&b, Block::dataSize);
            }
      break;
    }
}
