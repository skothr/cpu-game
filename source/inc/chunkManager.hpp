#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include "chunk.hpp"
#include "chunkLoader.hpp"
#include "mesh.hpp"
#include "threadPool.hpp"
#include "terrain.hpp"

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

class cChunkManager
{
public:
  cChunkManager(const int numThreads, const Point3i &center, const Vector3i &loadRadius);
  ~cChunkManager();

  bool setWorld(const std::string &worldName);
  void start();
  void stop();

  Point3i getCenter() const;
  Vector3i getRadius() const;

  int getHeightAt(const Point3i &hPos);

  void setSeed(uint32_t seed);
  
  Point3i chunkPoint(const Point3i &worldPos) const;
  void setRadius(const Vector3i &loadRadius);
  void setCenter(const Point3i &newCenter);
  int numLoaded() const;
  
  block_t get(const Point3i &wp);
  //cBlock* get(const Point3i &wp);
  void set(const Point3i &wp, block_t type);
  void clear();
  void saveChunks();

  Point3i minChunk() const;
  Point3i maxChunk() const;

  void initGL(cShader *shader);
  void cleanupGL();
  void renderSimple(cShader *shader, Matrix4 pvm);
  //void renderComplex(cShader *shader, Matrix4 pvm);

  void update();
  void updateChunks(int id);

  friend std::ostream& operator<<(std::ostream &os, const cChunkManager &set);
  
private:
  struct ChunkData
  {
    ChunkData(const Point3i &chunkPos)
      : chunk(new cChunk(chunkPos)), mesh(), meshDirty(false), unload(false), active(false)
    {
      processing.store(false);
    }
    ~ChunkData()
    { delete chunk; }
    cChunk *chunk;
    cMesh mesh;
    //std::mutex lock;
    std::atomic<bool> processing;
    std::atomic<bool> meshDirty;
    std::atomic<bool> unload;
    std::atomic<bool> active;
  };
  std::vector<ChunkData*> mChunks;

  std::condition_variable mChunkCv;
  std::mutex mChunkLock;

  int mNumLoaded = 0;
  Point3i mCenter;
  Vector3i mLoadRadius;
  Vector3i mChunkDim;
  Point3i mMinChunk;
  Point3i mMaxChunk;
  Point3i mCenterShift; // offsets mChunks when the center chunk changes.
  
  double mSaveTimer = 0.0;
  cChunkLoader mLoader;
  cThreadPool mPool;
  //cThreadPool mMeshPool;

  cPerlinNoise mNoise;
  uint32_t mSeed = 0;
  
  Point3i getArrayPos(const Point3i &cp) const;
  Point3i getRelativePos(const Point3i &ap) const;

  void updateMeshWorker(int id);
  void updateWorker(int id);
  void chunkLoadCallback(const Point3i &chunkPos);
  void chunkSaveCallback(const Point3i &chunkPos);
  
  void clearMesh(int chunkIndex);
  //void updateMesh(int chunkIndex);
  void updateMesh(ChunkData *chunk, const Point3i &chunkPos);

  Point3i getShifted(const Point3i &cp) const;

  // TODO: Move to terrain
  void generateChunk(const Point3i &chunkPos, terrain_t genType,
                     std::vector<uint8_t> &dataOut);
  
  // optimized functions for indexing
  static int index(int cx, int cy, int cz, int sx, int sz);
  int index(int cx, int cy, int cz) const;
  int index(const Point3i &cp) const;
  Point3i unflattenIndex(int index) const;
  int chunkX(int wx) const;
  int chunkY(int wy) const;
  int chunkZ(int wz) const;
  Point3i chunkPos(const Point3i &wp) const;

  int adjPX(int ci) const;
  int adjPY(int ci) const;
  int adjPZ(int ci) const;

  int adjNX(int ci) const;
  int adjNY(int ci) const;
  int adjNZ(int ci) const;
  int blockShiftX(int ci, int dx) const;
  int blockShiftY(int ci, int dy) const;
  int blockShiftZ(int ci, int dz) const;

  cBlock* adjBlock(int ci, int bx, int by, int bz, blockSide_t side);

  blockSide_t adjActive(int ci, int bx, int by, int bz, blockSide_t cSides);
  blockSide_t adjacentLoaded(int ci) const;
};



#endif // CHUNK_MANAGER_HPP
