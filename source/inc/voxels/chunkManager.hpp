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
#include <deque>
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


  void setLighting(int lightLevel);
  void updateLighting();
  
  void setRadius(const Vector3i &loadRadius);
  void setCenter(const Point3i &newCenter);
  Point3i getCenter() const;
  Vector3i getRadius() const;
  Point3i minChunk() const;
  Point3i maxChunk() const;
  
  block_t get(const Point3i &wp);
  cBlock* at(const Point3i &wp);
  
  void set(const Point3i &wp, block_t type);
  int numLoaded() const;
  int getHeightAt(const Point3i &hPos);
  
  void clear();
  void saveChunks();

  void update();
  void updateMeshes(int id);
  void loadCallback(Chunk* &&chunk);

  Point3i chunkPoint(const Point3i &worldPos) const;
  friend std::ostream& operator<<(std::ostream &os, const cChunkManager &set);
  
  static int chunkX(int wx);
  static int chunkY(int wy);
  static int chunkZ(int wz);
  static Point3i chunkPos(const Point3i &wp);
  
private:
  /*
  std::mutex mChunkLock;
  cChunkArray mChunks;
  //std::unordered_map<int, cChunk*> mChunks;
  //std::deque<cChunk*> mInactiveChunks;
  //std::vector<std::queue<cChunk*>> mNewChunks;
  //std::vector<std::queue<int>> mDeadChunks;
  //std::vector<std::vector<Point3i>> mThreadLoop;

  bool mLightInit = false;
  std::atomic<int> mNumLoaded = 0;
  Point3i mCenter;
  Vector3i mLoadRadius;
  Vector3i mChunkDim;
  Point3i mMinChunk;
  Point3i mMaxChunk;

  int mLighting = 0;
  
  cChunkLoader mLoader;
  ThreadPool mUpdatePool;

  // rendering
  cShader *mBlockShader = nullptr;
  cTextureAtlas *mTexAtlas = nullptr;

  Point3f mCamPos;
  */

  void updateWorker(int id);
  void chunkLoadCallback(Chunk* chunk);
};



#endif // CHUNK_MANAGER_HPP
