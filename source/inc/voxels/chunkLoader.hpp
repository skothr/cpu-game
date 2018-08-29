#ifndef CHUNK_LOADER_HPP
#define CHUNK_LOADER_HPP

#include "block.hpp"
#include "chunk.hpp"
#include "threadPool.hpp"
#include "threadQueue.hpp"
#include "terrain.hpp"
#include "worldFile.hpp"

#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <memory>

class cRegionFile;

// attaches to file and loads/saves chunks as requested.
class cChunkLoader
{
public:
  static const std::unordered_set<uint32_t> acceptedVersions;
  typedef std::function<void(Chunk* chunk)> loadCallback_t;
  
  cChunkLoader(int loadThreads, const loadCallback_t &loadCallback);
  ~cChunkLoader();

  void start();
  void stop();
  bool isRunning() const { return mLoadPool.running(); }

  bool createWorld(const std::string &worldName = "", uint32_t seed = 0);
  bool loadWorld(const std::string &worldName = "");
  
  std::vector<std::string> listWorlds();
  std::vector<std::string> listRegions(const std::string &worldDir);
  
  void load(Chunk *chunk);
  void loadDirect(Chunk *chunk); // loads chunk in calling thread
  void save(Chunk *chunk);


  uint32_t getSeed() const { return mHeader.seed; }
  
private:
  // world file
  std::string mWorldName = "";
  std::string mWorldPath = "";
  wDesc::Header mHeader;
  // region file(s)
  std::mutex mRegionLock;
  std::unordered_map<uint32_t, cRegionFile*> mRegionLookup;
  // threading
  loadCallback_t mLoadCallback;
  ThreadPool mLoadPool;
  ThreadQueue<Chunk> mLoadQueue;
  // other
  cTerrainGenerator mTerrainGen;
  
  bool checkVersion(const Vector<uint8_t, 4> &version) const;

  void loadChunk(Chunk *chunk);
  void loadWorker(int tid);
};




#endif // CHUNK_LOADER_HPP
