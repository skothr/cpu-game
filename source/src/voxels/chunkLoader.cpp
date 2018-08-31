#include "chunkLoader.hpp"
#include "logging.hpp"
#include "hashing.hpp"
#include "regionFile.hpp"
#include <iostream>
#include <filesystem>
#include <stddef.h>

#define WORLD_DIR "worlds/"

namespace fs = std::filesystem;

const std::unordered_set<uint32_t> ChunkLoader::acceptedVersions
{ 0x00000100 };

bool ChunkLoader::checkVersion(const Vector<uint8_t, 4> &version) const
{
  if(acceptedVersions.find(((int)version[3]) |
                           ((int)version[2] << 8) |
                           ((int)version[1] << 16) |
                           ((int)version[0] << 24) ) == acceptedVersions.end() )
    {
      LOGE("Unsupported version!");
      return false;
    }
  else
    { return true; }
}


#define LOAD_SLEEP_US 1000
#define SAVE_SLEEP_US 1000


ChunkLoader::ChunkLoader(int loadThreads, const loadCallback_t &loadCallback)
  : mLoadPool(loadThreads, std::bind(&ChunkLoader::loadWorker, this,
                                     std::placeholders::_1 ), LOAD_SLEEP_US ),
    mLoadCallback(loadCallback), mTerrainGen(0)
{ }

ChunkLoader::~ChunkLoader()
{
  stop();
  std::lock_guard<std::mutex> lock(mRegionLock);
  for(auto rFile : mRegionLookup)
    {
      delete rFile.second;
    }
  mRegionLookup.clear();
}

void ChunkLoader::stop()
{ mLoadPool.stop(); }
void ChunkLoader::start()
{ mLoadPool.start(); }

std::vector<std::string> ChunkLoader::listWorlds()
{
  // create global world directory if needed
  if(!fs::exists(WORLD_DIR))
    {
      LOGI("Creating global world directory ('%s')", WORLD_DIR);
      fs::create_directory(WORLD_DIR);
    }
  
  std::vector<std::string> worlds;
  for(auto &p : fs::directory_iterator(WORLD_DIR))
    { worlds.push_back(p.path()); }
  return worlds;
}
std::vector<std::string> ChunkLoader::listRegions(const std::string &worldDir)
{
  std::vector<std::string> regions;
  for(auto& p: fs::directory_iterator(worldDir))
    {
      if(p.path().extension() == ".wr")
        { regions.push_back(p.path()); }
    }
  return regions;
}

bool ChunkLoader::createWorld(const std::string &worldName, uint32_t seed)
{
  // create global world directory if needed
  if(!fs::exists(WORLD_DIR))
    {
      LOGI("Creating global world directory ('%s')", WORLD_DIR);
      fs::create_directory(WORLD_DIR);
    }
  
  const std::string worldDir = WORLD_DIR + worldName + "/";
  for(auto &w : listWorlds())
    {
      if(w == worldName)
        {
          std::stringstream promptss;
          promptss << "WARNING: Overwriting world '" << worldName
                   << "'. Do you want to continue?";
          if(!promptUserYN(promptss.str(), false))
            { return false; }
          else
            {
              std::cout << "Deleting world... :(\n";
              fs::remove_all(worldDir);
            }
          break;
        }
    }

  LOGI("Creating world...");
  fs::create_directory(worldDir);
  const std::string worldPath = worldDir + ".world";

  std::ofstream worldFile(worldPath, std::ios::out | std::ios::binary);
  if(!worldFile.is_open())
    {
      fs::remove(worldPath);
      LOGE("Failed to create world file!");
      return false;
    }
  // write header
  wDesc::Header worldHeader(Vector<uint8_t, 4>{0,0,1,0}, Block::dataSize, terrain_t::PERLIN, seed);
  worldFile.write(reinterpret_cast<char*>(&worldHeader), sizeof(wDesc::Header));
  
  return true;
}

bool ChunkLoader::loadWorld(const std::string &worldName)
{
  // create global world directory if needed
  if(!fs::exists(WORLD_DIR))
    {
      LOGI("Creating global world directory ('%s')", WORLD_DIR);
      fs::create_directory(WORLD_DIR);
    }
  
  if(worldName == "")
    { return false; }
  
  const std::string worldDir = WORLD_DIR + worldName + "/";
  if(!fs::exists(worldDir))
    { return false; }
  else
    { LOGI("Found  world!\n"); }

  // open world file and read info
  mWorldName = worldName;
  mWorldPath = worldDir + ".world";
  std::ifstream worldFile(mWorldPath, std::ios::in | std::ios::binary);
  if(!worldFile.is_open())
    { return false; }
  worldFile.read(reinterpret_cast<char*>(&mHeader), sizeof(wDesc::Header));
  worldFile.close();
  LOGI("  World file version: %d.%d.%d.%d", mHeader.version[0], mHeader.version[1],
       mHeader.version[2], mHeader.version[3] );

  if(!checkVersion(mHeader.version))
    { return false; }

  mTerrainGen.setSeed(mHeader.seed);
  
  // open region files
  std::vector<std::string> regions = listRegions(worldDir);
  std::lock_guard<std::mutex> rlock(mRegionLock);
  mRegionLookup.clear();
  for(int i = 0; i < regions.size(); i++)
    {
      LOGD("FOUND REGION: %s", regions[i].c_str());
      RegionFile *rFile = new RegionFile(regions[i], mHeader.version, false);
      if(!(*rFile))
        {
          LOGE("FAILED TO LOAD REGION FILE %s!!!", regions[i].c_str());
          exit(1);
        }
      if(!(*rFile))
        {
          LOGW("Failed to open region file!");
          delete rFile;
          for(auto &f : mRegionLookup)
            {
              f.second->close();
              delete f.second;
            }
          return false;
        }
      mRegionLookup[Hash::hash(rFile->getRegionPos())] = rFile;
    }
  return true;
}

void ChunkLoader::load(Chunk* chunk)
{
  //LOGD("CHUNK LOADER RECV LOAD REQUEST");
  mLoadQueue.push(chunk);
}
void ChunkLoader::loadWorker(int tid)
{
  Chunk* chunk = mLoadQueue.pop();
  
  if(chunk)
    {
      loadDirect(chunk);
      //LOGD("CALLING LOAD CALLBACK!");
      mLoadCallback(chunk);
    }
}

void ChunkLoader::loadDirect(Chunk* chunk)
{
  const Point3i cPos = chunk->pos();
  const Point3i rPos({ cPos[0] >> 4,
                       cPos[1] >> 4,
                       cPos[2] >> 4 });
  
  RegionFile *rFile = nullptr;
  {
    std::lock_guard<std::mutex> rlock(mRegionLock);
    auto iter = mRegionLookup.find(Hash::hash(rPos));
    if(iter != mRegionLookup.end())
      {
        rFile = iter->second;
      }
  }
      
  if(!rFile || !rFile->readChunk(chunk))
    { // chunk not in file -- needs to be generated.
      //LOGD("GENERATING CHUNK...");
      std::vector<uint8_t> chunkData;
      mTerrainGen.generate(chunk->pos(), mHeader.terrain, chunkData);
      chunk->deserialize(chunkData);
    }
}

void ChunkLoader::save(Chunk* chunk)
{
  const Point3i cPos = chunk->pos();
  const Point3i rPos({ cPos[0] >> 4,
                       cPos[1] >> 4,
                       cPos[2] >> 4 });
      
  RegionFile *rFile = nullptr;
  {
    std::lock_guard<std::mutex> rlock(mRegionLock);
    auto iter = mRegionLookup.find(Hash::hash(rPos));
    if(iter != mRegionLookup.end())
      { rFile = iter->second; }
  }

  if(!rFile)
    { // create new region file
      rFile = new RegionFile(WORLD_DIR + mWorldName + "/" + RegionFile::regionFileName(rPos),
                              mHeader.version, true );
      if(!(*rFile))
        {
          LOGE("Failed to create new region file!!");
          exit(1);
        }
      std::lock_guard<std::mutex> rlock(mRegionLock);
      mRegionLookup[Hash::hash(rPos)] = rFile;
    }

  if(!rFile->writeChunk(chunk))
    {
      LOGE("FAILED TO SAVE CHUNK TO REGION FILE!!!");
      exit(1); // TODO: Skip failed once this is all stable.
    }
}
