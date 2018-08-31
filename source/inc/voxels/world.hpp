#ifndef WORLD_HPP
#define WORLD_HPP

#include "block.hpp"
#include "chunkManager.hpp"
#include "chunkLoader.hpp"
#include "chunkMesh.hpp"
#include "threadQueue.hpp"
#include "threadPool.hpp"
#include "miscMath.hpp"

#include <mutex>
#include <atomic>
#include <unordered_map>
#include <unordered_set>

class Shader;
class TexAtlas;

class World
{
public:
  World(int loadThreads, const Point3i &center, const Vector3i &chunkRadius);
  ~World();

  // loading
  bool loadWorld(std::string worldName, uint32_t seed);
  void startLoading();
  void stopLoading();

  // rendering
  void initGL(QObject *qparent);
  void cleanupGL();
  void render(Matrix4 pvm);

  void setDebug(bool debug) { mDebug = debug; }
  void setFluidSim(bool on) { mSimFluids = on; }
  void setFluidEvap(bool on) { mEvapFluids = on; }

  // updating
  void update();
  void step();
  bool readyForPlayer() const;
  Point3f getStartPos(const Point3i &pPos);
  
  void setLightLevel(int level) { mLighting = level; }
  void setCenter(const Point3i &chunkCenter);
  Point3i getCenter() const;
  void setCamPos(const Point3f &pos) { mCamPos = pos; }

  Block* at(const Point3i &wp);
  BlockData* getData(const Point3i &wp);
  block_t getType(const Point3i &wp);
  Chunk* getChunk(const Point3i &wp);
  bool setBlock(const Point3i &p, block_t type, BlockData *data = nullptr);
  
  bool rayCast(const Point3f &p, const Vector3f &d, float radius,
	       CompleteBlock &blockOut, Point3i &posOut, Vector3i &faceOut );
  
  static int chunkX(int wx);
  static int chunkY(int wy);
  static int chunkZ(int wz);
  static Point3i chunkPos(const Point3i &wp);
  
private:
  bool mInitialized = false;
  bool mDebug = false;
  bool mSimFluids = true;
  bool mEvapFluids = true;
  
  ChunkLoader mLoader;
  Point3i mCenter;
  Vector3i mLoadRadius;
  Vector3i mChunkDim;
  Point3i mMinChunk;
  Point3i mMaxChunk;
  
  int mLighting = 0;
  Point3f mCamPos;

  // chunk updating
  std::mutex mChunkLock;
  std::atomic<int> mNumLoaded = 0;
  ThreadQueue<Chunk> mLoadQueue;
  ThreadQueue<Chunk> mUnusedChunks;
  std::unordered_map<int32_t, Chunk*> mChunks;
  std::unordered_map<int32_t, int> mNeighborsLoaded;
  centerLoopCallback_t mCenterLoopCallback;

  // meshing
  struct MeshedChunk
  {
    int32_t hash = 0;
    MeshData mesh;
  };
  ThreadPool mMeshPool;
  //ThreadQueue<MeshedChunk> mUnusedMeshData;
  std::deque<std::pair<int32_t, Chunk*>> mMeshQueue;
  std::unordered_set<int32_t> mMeshing;
  std::condition_variable mMeshCv;
  std::mutex mMeshLock;
  
  // rendering
  std::mutex mRenderLock;
  Shader *mBlockShader = nullptr;
  Shader *mChunkLineShader = nullptr;
  cTextureAtlas *mTexAtlas = nullptr;
  
  ThreadQueue<MeshedChunk> mRenderQueue;
  std::unordered_set<int32_t> mUnloadMeshes;
  ThreadQueue<ChunkMesh> mUnusedMeshes;
  std::unordered_map<int32_t, ChunkMesh*> mRenderMeshes;
  cMeshBuffer* mChunkLineMesh;
  
  Block* atBlock(const Point3i &wp, std::unordered_map<int32_t, Chunk*> &neighbors);
  block_t getBlock(const Point3i &wp, std::unordered_map<int32_t, Chunk*> &neighbors);
  void updateAdjacent(const Point3i &wp);

  void updateNeighbors(Chunk *chunk);
  int neighborsLoaded(Chunk *chunk);
  void chunkLoadCallback(Chunk *chunk);
  bool checkChunkLoad(const Point3i &cp);

  void meshWorker(int tid);
  int getAO(int e1, int e2, int c);
  int getLighting(const Point3i &bp, const Point3f &vpos, blockSide_t side,
                  std::unordered_map<int32_t, Chunk*> &neighbors );
  void updateChunkMesh(Chunk *chunk, bool priority);
  void addMesh(MeshedChunk *mc);

  MeshData makeChunkLineMesh();
  
  //bool correctChunk(const Point3i &bp, const Point3i &bi, Chunk *chunk, Chunk* &chunkOut, Point3i &pOut);
  //bool updateChunkEdges(Chunk *chunk);

};

#endif // WORLD_HPP
