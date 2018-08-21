#include "chunkLoader.hpp"
#include "logging.hpp"
#include "regionFile.hpp"
#include <iostream>
#include <filesystem>
#include <stddef.h>

#define WORLD_DIR "worlds/"

namespace fs = std::filesystem;

const std::unordered_set<uint32_t> cChunkLoader::acceptedVersions
{ 0x00000100 };

bool cChunkLoader::checkVersion(const Vector<uint8_t, 4> &version) const
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


cChunkLoader::cChunkLoader(int loadThreads, const loadCallback_t &loadCallback)
  : mLoadPool(loadThreads, std::bind(&cChunkLoader::checkLoad, this,
                                     std::placeholders::_1 ), LOAD_SLEEP_US ),
    mLoadCallback(loadCallback), mTerrainGen(0)
    //mSavePool(1, std::bind(&cChunkLoader::checkSave, this, // ALWAYS just one save thread
    //                     std::placeholders::_1 ), SAVE_SLEEP_US )
{ }

cChunkLoader::~cChunkLoader()
{
  stop();
  std::lock_guard<std::mutex> lock(mRegionLock);
  for(auto rFile : mRegionLookup)
    {
      delete rFile.second;
    }
  mRegionLookup.clear();
}

void cChunkLoader::stop()
{
  mLoadPool.stop();
  //mSavePool.stop();
}
void cChunkLoader::start()
{
  mLoadPool.start();
  //mSavePool.start();
}

std::vector<std::string> cChunkLoader::listWorlds()
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
std::vector<std::string> cChunkLoader::listRegions(const std::string &worldDir)
{
  std::vector<std::string> regions;
  for(auto& p: fs::directory_iterator(worldDir))
    {
      if(p.path().extension() == ".wr")
        { regions.push_back(p.path()); }
    }
  return regions;
}

bool cChunkLoader::createWorld(const std::string &worldName, uint32_t seed)
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
  wDesc::Header worldHeader(Vector<uint8_t, 4>{0,0,1,0}, cBlock::dataSize, terrain_t::PERLIN_CHUNK, seed);
  worldFile.write(reinterpret_cast<char*>(&worldHeader), sizeof(wDesc::Header));
  
  return true;
}

bool cChunkLoader::loadWorld(const std::string &worldName)
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
      cRegionFile *rFile = new cRegionFile(regions[i], mHeader.version, false);
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
      mRegionLookup[hashRegion(rFile->getRegionPos())] = rFile;
    }
  return true;
}

void cChunkLoader::load(chunkPtr_t chunk)
{
  LOGD("CHUNK LOADER RECV LOAD REQUEST");
  std::lock_guard<std::mutex> lock(mLoadLock);
  mLoadQueue.push(chunk);
}

void cChunkLoader::loadDirect(chunkPtr_t chunk)
{
  //LOGD("CHUNK LOADER LOAD DIRECT");
  const Point3i cPos = chunk->pos();
  const Point3i rPos({ cPos[0] >> 4,
                       cPos[1] >> 4,
                       cPos[2] >> 4 });

  //LOGD("READING CHUNK FROM REGION: %d,%d,%d", rPos[0], rPos[1], rPos[2]);
  
  cRegionFile *rFile = nullptr;
  {
    //LOGD("CHUNK LOADER FINDING REGION");
    std::lock_guard<std::mutex> rlock(mRegionLock);
    auto iter = mRegionLookup.find(hashRegion(rPos));
    if(iter != mRegionLookup.end())
      { rFile = iter->second; }
  }

      
  if(!rFile || !rFile->readChunk(chunk))
    { // chunk not in file -- needs to be generated.
      //LOGD("File: %d, read success: %d", (long)rFile, false);
      //LOGD("CHUNK LOADER GENERATING");
      std::vector<uint8_t> chunkData(cChunk::totalSize * cBlock::dataSize);
      mTerrainGen.generate(chunk->pos(), mHeader.terrain, chunkData);
      chunk->deserialize(chunkData.data(), chunkData.size());
    }
  else
    {
      //LOGD("File: %d, read success: %d", (long)rFile, true);
      //LOGD("READ CHUNK FROM FILE!!!!!!");
    }
  //LOGD("CHUNK LOADER DONE LOADING");
}

void cChunkLoader::saveDirect(chunkPtr_t chunk)
{
  const Point3i cPos = chunk->pos();
  const Point3i rPos({ cPos[0] >> 4,
                       cPos[1] >> 4,
                       cPos[2] >> 4 });
      
  //LOGD("WRITING CHUNK TO REGION: %d,%d,%d", rPos[0], rPos[1], rPos[2]);
  
  cRegionFile *rFile = nullptr;
  {
    std::lock_guard<std::mutex> rlock(mRegionLock);
    auto iter = mRegionLookup.find(hashRegion(rPos));
    if(iter != mRegionLookup.end())
      { rFile = iter->second; }
  }

  if(!rFile)
    { // create new region file
      //LOGD("CHUNK LOADER CREATING REGION");
      rFile = new cRegionFile(WORLD_DIR + mWorldName + "/" + cRegionFile::regionFileName(rPos),
                              mHeader.version, true );
      if(!(*rFile))
        {
          LOGE("Failed to create new region file!!");
          exit(1);
        }
      std::lock_guard<std::mutex> rlock(mRegionLock);
      mRegionLookup[hashRegion(rPos)] = rFile;
    }

  //LOGD("CHUNK LOADER WRITING CHUNK");
  if(!rFile->writeChunk(chunk))
    {
      LOGE("FAILED TO SAVE CHUNK TO REGION FILE!!!");
      exit(1); // TODO: Skip failed once this is all stable.
    }
}

/*
void cChunkLoader::save(chunkPtr_t chunk)
{
  // LOGD("CHUNK LOADER RECV SAVE REQUEST");
  // std::lock_guard<std::mutex> lock(mSaveLock);
  // mSaveQueue.push(chunk);
  
  const Point3i cPos = chunk->pos();
  const Point3i rPos({ cPos[0] / REGION_SIZEX,
                       cPos[1] / REGION_SIZEY,
                       cPos[2] / REGION_SIZEZ });
      
  cRegionFile *rFile = nullptr;
  {
    std::lock_guard<std::mutex> rlock(mRegionLock);
    auto iter = mRegionLookup.find(hashRegion(rPos));
    if(iter != mRegionLookup.end())
      { rFile = iter->second; }
  }

  if(!rFile)
    { // create new region file
      LOGD("CHUNK LOADER THREAD CREATING REGION");
      rFile = new cRegionFile(WORLD_DIR + mWorldName + cRegionFile::regionFileName(rPos),
                              mHeader.version, true );
      if(!(*rFile))
        {
          LOGE("Failed to create new region file!!");
          exit(1);
        }
      std::lock_guard<std::mutex> rlock(mRegionLock);
      mRegionLookup[hashRegion(rPos)] = rFile;
    }

  LOGD("CHUNK LOADER THREAD WRITING CHUNK");
  if(!rFile->writeChunk(chunk->ptr()))
    {
      LOGE("FAILED TO SAVE CHUNK TO REGION FILE!!!");
      exit(1); // TODO: Skip failed once this is all stable.
    }

  // done with chunk
  chunk->setDirty(false);
  chunk->setLoading(false);
}
*/
/*
chunkPtr_t&& cChunkLoader::getLoadChunk()
{
  chunkPtr_t chunk = nullptr;
  std::lock_guard<std::mutex> lock(mLoadLock);
  if(mLoadQueue.size() > 0)
    {
      chunk = mLoadQueue.front();
      mLoadQueue.pop();
    }
  return std::move(chunk);
}
chunkPtr_t&& cChunkLoader::getSaveChunk()
{
  chunkPtr_t chunk = nullptr;
  std::lock_guard<std::mutex> lock(mSaveLock);
  if(mSaveQueue.size() > 0)
    {
      chunk = mSaveQueue.front();
      mSaveQueue.pop();
    }
  return std::move(chunk);
}
*/
void cChunkLoader::checkLoad(int tid)
{
  chunkPtr_t chunk = nullptr;
  std::lock_guard<std::mutex> lock(mLoadLock);
  if(mLoadQueue.size() > 0)
    {
      chunk = std::move(mLoadQueue.front());
      mLoadQueue.pop();
    }
  if(chunk)
    {
      LOGD("CHUNK LOADER THREAD LOAD");
      const Point3i cPos = chunk->pos();
  const Point3i rPos({ cPos[0] >> 4,
                       cPos[1] >> 4,
                       cPos[2] >> 4 });
      cRegionFile *rFile = nullptr;
      {
        std::lock_guard<std::mutex> rlock(mRegionLock);
        auto iter = mRegionLookup.find(hashRegion(rPos));
        if(iter != mRegionLookup.end())
          { rFile = iter->second; }
      }
      
      if(rFile)
        {
          LOGD("CHUNK LOADER THREAD GOT REGION");
          if(!rFile->readChunk(chunk))
            { // chunk not in file -- needs to be generated.
              std::vector<uint8_t> chunkData(cChunk::totalSize * cBlock::dataSize);
              mTerrainGen.generate(chunk->pos(), mHeader.terrain, chunkData);
              chunk->deserialize(chunkData.data(), chunkData.size());
            }
        }
      // done with chunk
      //chunk->setLoading(false);
      mLoadCallback(chunk);
    }
}
/*
void cChunkLoader::checkSave(int tid)
{
  chunkPtr_t chunk = getSaveChunk();
  if(chunk && chunk->isDirty())
    {
      const Point3i cPos = chunk->pos();
      const Point3i rPos({ cPos[0] / REGION_SIZEX,
                           cPos[1] / REGION_SIZEY,
                           cPos[2] / REGION_SIZEZ });
      
      cRegionFile *rFile = nullptr;
      {
        std::lock_guard<std::mutex> rlock(mRegionLock);
        auto iter = mRegionLookup.find(hashRegion(rPos));
        if(iter != mRegionLookup.end())
          { rFile = iter->second; }
      }

      if(!rFile)
        { // create new region file
          LOGD("CHUNK LOADER THREAD CREATING REGION");
          rFile = new cRegionFile(WORLD_DIR + mWorldName + cRegionFile::regionFileName(rPos),
                                  mHeader.version, true );
          if(!(*rFile))
            {
              LOGE("Failed to create new region file!!");
              exit(1);
            }
          std::lock_guard<std::mutex> rlock(mRegionLock);
          mRegionLookup[hashRegion(rPos)] = rFile;
        }

      LOGD("CHUNK LOADER THREAD WRITING CHUNK");
      if(!rFile->writeChunk(chunk))
        {
          LOGE("FAILED TO SAVE CHUNK TO REGION FILE!!!");
          exit(1); // TODO: Skip failed once this is all stable.
        }

      // done with chunk
      chunk->setDirty(false);
      chunk->setLoading(false);
    }
}
*/
inline int cChunkLoader::expand(int x)
{
  x                  &= 0x000003FF;
  x  = (x | (x<<16)) &  0xFF0000FF;
  x  = (x | (x<<8))  &  0x0F00F00F;
  x  = (x | (x<<4))  &  0xC30C30C3;
  x  = (x | (x<<2))  &  0x49249249;
  return x;
}
inline int cChunkLoader::unexpand(int x) const
{
  x                 &= 0x49249249;
  x = (x | (x>>2))  &  0xC30C30C3;
  x = (x | (x>>4))  &  0x0F00F00F;
  x = (x | (x>>8))  &  0xFF0000FF;
  x = (x | (x>>16)) &  0x000003FF;
  return (x << 22) >> 22;
}
inline int cChunkLoader::hashRegion(int cx, int cy, int cz)
{ return expand(cx) + (expand(cy) << 1) + (expand(cz) << 2); }
inline int cChunkLoader::hashRegion(const Point3i &cp)
{ return hashRegion(cp[0], cp[1], cp[2]); }
inline int cChunkLoader::unhashX(int cx) const
{ return unexpand(cx); }
inline int cChunkLoader::unhashY(int cy) const
{ return unexpand(cy); }
inline int cChunkLoader::unhashZ(int cz) const
{ return unexpand(cz); }
inline Point3i cChunkLoader::unhash(const Point3i &rp) const
{ return Point3i{unexpand(rp[0]), unexpand(rp[1]), unexpand(rp[2])}; }
