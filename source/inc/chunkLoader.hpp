#ifndef CHUNK_LOADER_HPP
#define CHUNK_LOADER_HPP

#include "block.hpp"
#include "chunk.hpp"
#include "threadPool.hpp"
#include "terrain.hpp"
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
#include <fstream>

class cRegionFile;

typedef cChunk* chunkPtr_t;

// attaches to file and loads/saves chunks as requested.
class cChunkLoader
{
public:
  typedef std::function<void(chunkPtr_t chunk)> loadCallback_t;
  //typedef std::function<void(const Point3i &chunkPos)> saveCallback_t;
  
  static const std::unordered_set<uint32_t> acceptedVersions;
  
  cChunkLoader(int loadThreads, const loadCallback_t &loadCallback);
		    //const saveCallback_t &saveCallback);
  ~cChunkLoader();

  void start();
  void stop();

  bool createWorld(const std::string &worldName = "", uint32_t seed = 0);
  bool loadWorld(const std::string &worldName = "");
  
  std::vector<std::string> listWorlds();
  std::vector<std::string> listRegions(const std::string &worldDir);
  
  void load(chunkPtr_t chunk);
  void save(chunkPtr_t chunk);

  void loadDirect(chunkPtr_t chunk);
  void saveDirect(chunkPtr_t chunk);

  uint32_t getSeed() const { return mHeader.seed; }
  
private:
  std::string mWorldName = "";
  std::string mWorldPath = "";
  wDesc::Header mHeader;

  std::mutex mRegionLock;
  std::unordered_map<uint32_t, cRegionFile*> mRegionLookup;

  cTerrainGenerator mTerrainGen;

  loadCallback_t mLoadCallback;
  cThreadPool mLoadPool;
  std::mutex mLoadLock;
  std::queue<chunkPtr_t> mLoadQueue;
  //cThreadPool mSavePool;
  //std::mutex mSaveLock;
  //std::queue<chunkPtr_t> mSaveQueue;
  
  bool checkVersion(const Vector<uint8_t, 4> &version) const;

  void loadChunk(chunkPtr_t chunk);
  //void saveChunk(chunkPtr_t &chunk);
  //cChunk* getLoadChunk();
  //cChunk* getSaveChunk();
  void checkLoad(int tid);
  //void checkSave(int tid);

  // optimized hashing functions
  int expand(int x);
  int unexpand(int x) const;
  int hashRegion(int cx, int cy, int cz);
  int hashRegion(const Point3i &cp);
  int unhashX(int cx) const;
  int unhashY(int cy) const;
  int unhashZ(int cz) const;
  Point3i unhash(const Point3i &rp) const;
};




#endif // CHUNK_LOADER_HPP
