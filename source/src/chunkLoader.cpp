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
  return (acceptedVersions.find(((int)version[3]) |
				((int)version[2] << 8) |
				((int)version[1] << 16) |
				((int)version[0] << 24) ) != acceptedVersions.end() );
}




cChunkLoader::cChunkLoader()
{ }

cChunkLoader::~cChunkLoader()
{
  //stop();
  std::lock_guard<std::mutex> lock(mRegionLock);
  for(auto rFile : mRegionLookup)
    {
      delete rFile.second;
    }
  mRegionLookup.clear();
}

void cChunkLoader::setSeed(uint32_t seed)
{
  mWorldHeader.seed = seed;
  if(mWorldFile.isOpen())
    {
      mWorldFile.write(0, reinterpret_cast<uint8_t*>(&mWorldHeader), sizeof(wDesc::Header));
    }
}

bool cChunkLoader::loadWorld(const std::string &worldName)
{
  if(worldName == "")
    {
      std::cout << "Enter world name: ";
      std::cin >> mWorldName;
    }
  else
    { mWorldName = worldName; }

  // create global world directory if needed
  if(!fs::exists(WORLD_DIR))
    {
      LOGI("Creating global world directory ('%s')", WORLD_DIR);
      fs::create_directory(WORLD_DIR);
    }
  
  // print existing worlds
  for(auto &p : fs::directory_iterator(WORLD_DIR))
    { std::cout << "(world: '" << p.path() << "')\n"; }

  
  // find directory for specified world
  const std::string worldDir = WORLD_DIR + mWorldName + "/";
  mWorldPath = worldDir + mWorldName + ".world";
  
  if(!fs::exists(worldDir))
    {
      std::cout << "World '" << mWorldName << "' not found. Would you like to create it? (y/n) ";
      bool create = false;
      while(true)
	{
	  std::string resp;
	  std::cin >> resp;
	  if(resp[0] == 'y' || resp[0] == 'Y')
	    {
	      create = true;
	      break;
	    }
	  else if(resp[0] == 'n' || resp[0] == 'N')
	    {
	      create = false;
	      break;
	    }
	  else
	    { std::cout << "Please enter 'y' or 'n'. "; }
	}

      if(create)
	{
	  std::cout << "Creating world...\n";
	  fs::create_directory(worldDir);
	  
	  if(!mWorldFile.open(mWorldPath, (cMmapFile::OUTPUT | cMmapFile::INPUT |
					   cMmapFile::TRUNC | cMmapFile::CREATE), true,
			      sizeof(wDesc::Header)))
	    {
	      LOGE("Failed to create world description file!");
              fs::remove(mWorldPath);
	      return false;
	    }

          mWorldHeader = wDesc::Header(Vector<uint8_t, 4>{0,0,1,0}, cBlock::dataSize,
                                       terrain_t::PERLIN, mSeed); //, mWorldName );
	  mWorldFile.write(0, reinterpret_cast<uint8_t*>(&mWorldHeader), sizeof(wDesc::Header));
	  mWorldFile.close();
	}
      else
	{
	  std::cout << "Cancelling...\n";
	  return false;
	}
    }
  else
    {
      std::cout << "Found world!\n";
    }

  LOGD("LOAD WORLD DONE");
  return readWorld();
}

bool cChunkLoader::readWorld()
{
  LOGD("READING WORLD FILE --> '%s'", mWorldPath.c_str());
  if(!mWorldFile.open(mWorldPath, cMmapFile::INPUT | cMmapFile::OUTPUT, true))
    {
      LOGE("Could not open world description file at '%s'!", mWorldPath);
      return false;
    }
  
  // read header
  mWorldFile.read(0, reinterpret_cast<uint8_t*>(&mWorldHeader), sizeof(wDesc::Header));
  // check version
  LOGI("  World file version: %d.%d.%d.%d", mWorldHeader.version[0], mWorldHeader.version[1],
       mWorldHeader.version[2], mWorldHeader.version[3] );
  if(!checkVersion(mWorldHeader.version))
    {
      LOGE("World decription file at '%s' has unsupported version!", mWorldPath);
      return false;
    }

  mSeed = mWorldHeader.seed;
  

  // find region files
  const std::string worldDir = WORLD_DIR + mWorldName + "/";
  std::vector<std::string> regionFiles;
  for(auto& p: fs::directory_iterator(worldDir))
    {
      if(p.path().extension() == ".wr")
        {
          std::cout << "  Region: " << p.path() << '\n';
          regionFiles.push_back(p.path());
        }
      else
        {
          std::cout << "skipped '" << p.path() << "\n";
        }
    }

  std::cout << "Num regions: " << regionFiles.size() << "\n";
  {
    std::lock_guard<std::mutex> rlock(mRegionLock);
    mRegionLookup.clear();
    for(int i = 0; i < regionFiles.size(); i++)
      {
        cRegionFile *rFile = new cRegionFile(regionFiles[i], mWorldHeader.version, false);
          if(!rFile || !(*rFile))
            {
             LOGW("Skipping region file '%s' (failed to open)", regionFiles[i].c_str());
             return false;
            }
        mRegionLookup[hashRegion(rFile->getRegionPos())] = rFile;
      }
  }

  return true;
}

std::string cChunkLoader::regionFilePath(uint32_t regionHash) const
{
  const Point3i regionPos({unhashX(regionHash),
                           unhashY(regionHash),
                           unhashZ(regionHash)});
  std::ostringstream ss;
  ss << WORLD_DIR << mWorldName << "/r."
     << regionPos[0] << "." << regionPos[1] << "." << regionPos[2]
     << ".wr";
  return ss.str();
}

bool cChunkLoader::load(cChunk *chunk)
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

  if(rFile)
    { return rFile->readChunk(chunk); }
  else
    { return false; }
}

bool cChunkLoader::save(const cChunk *chunk)
{
  const Point3i cPos = chunk->pos();
  const Point3i rPos({ cPos[0] / REGION_SIZEX,
                       cPos[1] / REGION_SIZEY,
                       cPos[2] / REGION_SIZEZ });
  const uint32_t rHash = hashRegion(rPos);
  cRegionFile *rFile = nullptr;
  {
    std::lock_guard<std::mutex> rlock(mRegionLock);
    auto iter = mRegionLookup.find(rHash);
    if(iter != mRegionLookup.end())
      { rFile = iter->second; }
  }

  if(!rFile)
    { // create new region file
      rFile = new cRegionFile(regionFilePath(rHash), mWorldHeader.version, true);
    }

  if(rFile && (*rFile))
    { return rFile->writeChunk(chunk); }
  else
    { return false; }
}

// NOTE: hash is invertible over domain (+/- 2^10)^3
inline int cChunkLoader::expand(int x)
{
  //  0000 0000 0000 0000 0000 0011 1111 1111 --> clamp to max value
  x                  &= 0x000003FF;
  //  1111 1111 0000 0000 0000 0000 1111 1111 --> separate upper 2 bits
  x  = (x | (x<<16)) &  0xFF0000FF;
  //  0000 1111 0000 0000 1111 0000 0000 1111 --> 
  x  = (x | (x<<8))  &  0x0F00F00F;
  //  1100 0011 0000 1100 0011 0000 1100 0011 --> 
  x  = (x | (x<<4))  &  0xC30C30C3;
  //  0100 1001 0010 0100 1001 0010 0100 1001 --> 
  x  = (x | (x<<2))  &  0x49249249;
  return x;
}
inline int cChunkLoader::hashRegion(int cx, int cy, int cz)
{ return expand(cx) + (expand(cy) << 1) + (expand(cz) << 2); }
inline int cChunkLoader::hashRegion(const Point3i &cp)
{ return hashRegion(cp[0], cp[1], cp[2]); }

inline int cChunkLoader::unexpand(int x) const
{
  x                 &= 0x49249249;
  x = (x | (x>>2))  &  0xC30C30C3;
  x = (x | (x>>4))  &  0x0F00F00F;
  x = (x | (x>>8))  &  0xFF0000FF;
  x = (x | (x>>16)) &  0x000003FF;
  return (x << 22) >> 22;
}
inline int cChunkLoader::unhashX(int cx) const
{ return unexpand(cx); }
inline int cChunkLoader::unhashY(int cy) const
{ return unexpand(cy); }
inline int cChunkLoader::unhashZ(int cz) const
{ return unexpand(cz); }

Point3i cChunkLoader::unhash(const Point3i &rp) const
{ return Point3i{unexpand(rp[0]), unexpand(rp[1]), unexpand(rp[2])}; }
