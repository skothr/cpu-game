#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include "chunk.hpp"
#include "chunkLoader.hpp"
#include "chunkMesh.hpp"
#include "threadPool.hpp"
#include "chunkArray.hpp"

#include <vector>
#include <unordered_map>
#include <queue>
#include <string>
#include <ostream>
#include <thread>
#include <mutex>
#include <condition_variable>

// NOTE:
//  - cx/cy/cz denotes chunk position
//  - px/py/pz denotes block position within world
//  - bx/by/bz denotes block position within chunk
//
//  - cp indicates chunk position
//  - wp indices world block position

#define WRITE_FREQUENCY 1.0 // seconds



class cShader;
class cTextureAtlas;

class cChunkManager
{
public:

  int getThread(const Point3i &cp);
  
  cChunkManager(int loadThreads, const Point3i &center, const Vector3i &loadRadius);
  ~cChunkManager();

  bool setWorld(std::string worldName, uint32_t seed = 0);
  void start();
  void stop();
  
  void initGL(QObject *qparent);
  void cleanupGL();
  void render(const Matrix4 &pvm);

  
  void setRadius(const Vector3i &loadRadius);
  void setCenter(const Point3i &newCenter);
  Point3i getCenter() const;
  Vector3i getRadius() const;
  Point3i minChunk() const;
  Point3i maxChunk() const;
  
  block_t get(const Point3i &wp);
  //cBlock* get(const Point3i &wp);
  void set(const Point3i &wp, block_t type);
  int numLoaded() const;
  int getHeightAt(const Point3i &hPos);
  
  void clear();
  void saveChunks();

  void update();
  void updateMeshes(int id);
  void loadCallback(chunkPtr_t &&chunk);

  Point3i chunkPoint(const Point3i &worldPos) const;
  friend std::ostream& operator<<(std::ostream &os, const cChunkManager &set);
  
private:
  std::mutex mChunkLock;
  cChunkArray mChunks;

  std::atomic<int> mNumLoaded = 0;
  Point3i mCenter;
  Vector3i mLoadRadius;
  Vector3i mChunkDim;
  Point3i mMinChunk;
  Point3i mMaxChunk;
  
  cChunkLoader mLoader;
  cThreadPool mUpdatePool;

  // rendering
  cShader *mBlockShader = nullptr;
  cTextureAtlas *mTexAtlas = nullptr;

  void updateWorker(int id);
  void chunkLoadCallback(chunkPtr_t chunk);
  
  // optimized hashing functions
  int expand(int x);
  int unexpand(int x) const;
  int hashChunk(int cx, int cy, int cz);
  int hashChunk(const Point3i &cp);
  int unhashX(int cx) const;
  int unhashY(int cy) const;
  int unhashZ(int cz) const;
  Point3i unhashChunk(const Point3i &rp) const;
  
  int chunkX(int wx) const;
  int chunkY(int wy) const;
  int chunkZ(int wz) const;
  Point3i chunkPos(const Point3i &wp) const;
};



#endif // CHUNK_MANAGER_HPP
