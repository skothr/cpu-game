#ifndef CHUNK_LOADER_HPP
#define CHUNK_LOADER_HPP

#include "block.hpp"
#include "chunk.hpp"
#include "threadPool.hpp"
#include "mmapFile.hpp"
#include "worldFile.hpp"

#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>
#include <mutex>
#include <queue>
#include <memory>

class cRegionFile;

// attaches to file and loads/saves chunks as requested.
class cChunkLoader
{
public:
  typedef std::function<void(const Point3i &chunkPos)> loadCallback_t;
  typedef std::function<void(const Point3i &chunkPos)> saveCallback_t;
  
  static const std::unordered_set<uint32_t> acceptedVersions;
  
  cChunkLoader();// numThreads, const loadCallback_t &loadCallback,
		    //const saveCallback_t &saveCallback);
  ~cChunkLoader();

  //void start();
  //void stop();

  bool loadWorld(const std::string &worldName = "");
  
  bool load(cChunk *chunk);
  bool save(const cChunk *chunk);

  void setSeed(uint32_t seed);
  uint32_t getSeed() const { return mSeed; }

  //void checkUpdate(int id);
  
private:
  std::string mWorldName = "";
  std::string mWorldPath = "";
  
  cMmapFile mWorldFile;
  wDesc::Header mWorldHeader;
  uint32_t mSeed = 0;

  std::mutex mRegionLock;
  std::unordered_map<uint32_t, cRegionFile*> mRegionLookup;
  /*
  cThreadPool mPool;
  std::mutex mLoadLock;
  std::mutex mSaveLock;
  std::queue<cChunk*> mLoadQueue;
  std::queue<cChunk*> mSaveQueue;
  loadCallback_t mLoadCallback;
  saveCallback_t mSaveCallback;
  */
  bool checkVersion(const Vector<uint8_t, 4> &version) const;
  bool readWorld();
  
  //uint32_t getRegionHash(const std::string &fileName) const;
  std::string regionFilePath(uint32_t regionHash) const;
  //int createRegion(uint32_t regionHash);
  //bool readRegion(const std::string &fileName, int rIndex = -1);

  void loadChunk(cChunk *chunk);
  void saveChunk(cChunk *chunk);
  //void generateChunk(const Point3i &chunkPos, terrain_t genType,
  //                   std::vector<uint8_t> &dataOut ) const;

  int expand(int x);
  int hashRegion(int cx, int cy, int cz);
  int hashRegion(const Point3i &cp);
  // optimized hashing functions
  int unexpand(int x) const;
  int unhashX(int cx) const;
  int unhashY(int cy) const;
  int unhashZ(int cz) const;
  Point3i unhash(const Point3i &rp) const;
};




#endif // CHUNK_LOADER_HPP
