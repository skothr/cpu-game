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


#define LOAD_SLEEP_US 10000


ChunkLoader::ChunkLoader(int loadThreads, const loadCallback_t &loadCallback)
  : mLoadPool(loadThreads, std::bind(&ChunkLoader::loadWorker, this,
                                     std::placeholders::_1 ), LOAD_SLEEP_US ),
    mLoadCallback(loadCallback), mTerrainGen(0)
{ }

ChunkLoader::~ChunkLoader()
{
  stop();
  for(auto rFile : mRegionLookup)
    { delete rFile.second; }
  mRegionLookup.clear();
}

void ChunkLoader::stop()
{
  mLoadPool.stop();
}
void ChunkLoader::start()
{ mLoadPool.start(); }
void ChunkLoader::flush()
{
  while(true)
    {
      Chunk *chunk = nullptr;
      {
        std::lock_guard<std::mutex> lock(mLoadLock);
        if(mLoadQueue.size() > 0)
          {
            chunk = mLoadQueue.front();
            mLoadQueue.pop();
          }
        else
          { break; }
      }
      if(chunk)
        { mLoadCallback(chunk); }
    }
}
bool ChunkLoader::checkWorldDir()
{ // create global world directory if needed
  if(!fs::exists(WORLD_DIR))
    {
      LOGI("Creating global world directory ('%s')", WORLD_DIR);
      fs::create_directory(WORLD_DIR);
      return true;
    }
  //LOGI("Directory already created...");
  return false;
}
std::vector<World::Options> ChunkLoader::getWorlds()
{
  if(checkWorldDir())
    { // created world dir so there definitely aren't any worlds
      return {};
    }
  std::vector<World::Options> worlds;
  for(auto &p : fs::directory_iterator(WORLD_DIR))
    {
      // open world file and read info
      const std::string path = p.path().string();
      const std::string worldPath = path + "/.world";
      const std::string worldName = path.substr(path.rfind(p.path().preferred_separator) + 1);
      std::cout << "WORLD PATH: " << worldPath << ", name: " << worldName << "\n";
      std::ifstream worldFile(worldPath, std::ios::in | std::ios::binary);
      if(!worldFile.is_open())
        {
          LOGW("Problem reading world file: '%s'", worldPath.c_str());
          continue;
        }
      wDesc::Header header;
      worldFile.read(reinterpret_cast<char*>(&header), sizeof(wDesc::Header));
      worldFile.close();
      
      LOGI("  World file version: %d.%d.%d.%d", header.version[0], header.version[1],
           header.version[2], header.version[3] );
      if(!checkVersion(header.version))
        {
          LOGW("World file with incorrect version: '%s'", worldPath.c_str());
          continue;
        }
      World::Options opt;
      opt.name = worldName;
      opt.terrain = header.terrain;
      opt.seed = header.seed;
      worlds.push_back(opt);
    }
  return worlds;
}
std::vector<std::string> ChunkLoader::listWorlds()
{
  if(checkWorldDir())
    { // created world dir so there definitely aren't any worlds
      return {};
    }
  std::vector<std::string> worlds;
  for(auto &p : fs::directory_iterator(WORLD_DIR))
    {
      const std::string path = p.path().string();
      const std::string worldName = path.substr(path.rfind(p.path().preferred_separator) + 1);
      worlds.push_back(worldName);
    }
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

bool ChunkLoader::createWorld(const std::string &worldName, terrain_t terrain, uint32_t seed)
{
  checkWorldDir();
  const std::string worldDir = WORLD_DIR + worldName + "/";
  for(auto &w : listWorlds())
    {
      if(w == worldName)
        {
          LOGW("Overwriting world... :(\n");
          fs::remove_all(worldDir);
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
  wDesc::Header worldHeader(Vector<uint8_t, 4>{0,0,1,0}, Block::dataSize, terrain, seed);
  worldFile.write(reinterpret_cast<char*>(&worldHeader), sizeof(wDesc::Header));
  
  return true;
}

bool ChunkLoader::loadWorld(const std::string &worldName)
{
  if(worldName == "")
    { return false; }
  
  checkWorldDir();
  const std::string worldDir = WORLD_DIR + worldName + "/";
  if(!fs::exists(worldDir))
    { return false; }

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
      LOGD("Found region: %s", regions[i].c_str());
      RegionFile *rFile = new RegionFile(regions[i], mHeader.version, false);
      if(!(*rFile))
        {
          LOGE("Failed to load region file '%s'", regions[i].c_str());
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
  std::lock_guard<std::mutex> lock(mLoadLock);
  mLoadQueue.push(chunk);
}
void ChunkLoader::loadWorker(int tid)
{
  Chunk* chunk = nullptr;
  {
    std::lock_guard<std::mutex> lock(mLoadLock);
    if(mLoadQueue.size() > 0)
      {
        chunk = mLoadQueue.front();
        mLoadQueue.pop();
      }
  }
  
  if(chunk)
    {
      loadDirect(chunk);
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
