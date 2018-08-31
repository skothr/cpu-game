#ifndef WORLD_FILE_HPP
#define WORLD_FILE_HPP
#pragma pack (push, 1)

#include "terrain.hpp"
#include "vector.hpp"
#include <cstdint>

#define REGION_SIZEX 16
#define REGION_SIZEY 16
#define REGION_SIZEZ 16
#define CHUNKS_PER_REGION (REGION_SIZEX * REGION_SIZEY * REGION_SIZEZ)
static const Point3i REGION_DIM{REGION_SIZEX, REGION_SIZEY, REGION_SIZEZ};


// // VERSION:   0.0.0.1
// // EXTENSION: *.bin
//
// // one chunk per entire infinite world (what could go wrong?)
// struct ChunkInfo; // (forward declaration)
//
// // FORMAT DATA
// struct FileHeader
// {
//   Vector<uint8_t, 4> version;  // ALWAYS FIRST 4 BYTES
//   uint8_t blockSize;
//   terrain_t terrain;
//   uint16_t numChunks;
// };
// // lookup table
// struct ChunkInfo
// {
//   uint32_t hashCode;
//   uint32_t fileOffset;
//   uint32_t chunkSize;
// }; // repeated once for every chunk in file
// // followed by chunk data


// VERSION:   0.0.1.0
// EXTENSION: *.wr

// chunks are broken up into region files, each containing data for up to 4096 chunks
// files stored in folder, with name correspoding to world name
// file names --> TODO
// folder also contains a general world description file


// WORLD DESCRIPTION DATA
namespace wDesc
{
  struct Header
  {
    Header(const Vector<uint8_t, 4> &version = {}, uint8_t blockSize = Block::dataSize,
           terrain_t terrain = terrain_t::INVALID, uint32_t seed = 0)//, const std::string &name = "" )
      : version(version), blockSize(blockSize), terrain(terrain), seed(seed)
    { }//std::memcpy((void*)worldName, (void*)name.c_str(),
      //          std::max(sizeof(wDesc::Header::worldName), name.size())); }
    
    Vector<uint8_t, 4> version; // ALWAYS FIRST 4 BYTES
    uint8_t blockSize;          // byte size of each block
    terrain_t terrain;          // terrain generation method
    //char worldName[32] = {0};         // name of world
    uint32_t seed;     // to protect end of worldName string (TODO: possisbly unnecessary)
  };
  // struct RegionDesc
  // {
  //   uint32_t hashCode = 0;        // code for region, based on chunk location (file name suffix)
  //   uint32_t numChunks = 0;      // number of chunks in region
  // }; // repeated once for every region file
}

// REGION DATA
namespace wData
{
  struct ChunkInfo; // (forward declaration)
  struct Header
  {
    Vector<uint8_t, 4> version; // ALWAYS FIRST 4 BYTES
    uint32_t nextOffset;
  };
  struct ChunkInfo
  {
    uint32_t offset = 0;    // byte offset of start of chunk data starting from end of lookup table
    uint32_t chunkSize = 0; // number of used bytes in chunk data
  }; // repeated once for every chunk
  // followed by chunk data
}

#define CHUNK_LOOKUP_SIZE (sizeof(wData::ChunkInfo) * CHUNKS_PER_REGION)






#pragma pack (pop)
#endif // WORLD_FILE_HPP
