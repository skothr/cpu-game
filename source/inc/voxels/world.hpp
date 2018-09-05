#ifndef WORLD_HPP
#define WORLD_HPP

#include "block.hpp"
#include "chunk.hpp"
#include "chunkMesh.hpp"
#include "threadQueue.hpp"
#include "threadPool.hpp"
#include "miscMath.hpp"
#include "frustum.hpp"
#include "meshing.hpp"
#include "terrain.hpp"
#include "fluidManager.hpp"

#include <mutex>
#include <atomic>
#include <vector>
#include <condition_variable>
#include <unordered_map>
#include <unordered_set>

class Shader;
class cTextureAtlas;
class ChunkLoader;
class FluidChunk;

class World
{
public:
  World();
  ~World();
  
  struct Options
  {
    std::string name;
    terrain_t terrain;
    uint32_t seed;
    Vector3i chunkRadius;
    int loadThreads;
    int meshThreads;
  };
  
  // loading
  std::vector<Options> getWorlds() const;
  bool worldExists(const std::string &name) const;
  bool loadWorld(const Options &opt);
  bool createWorld(const Options &opt, bool overwrite = false);
  void updateInfo(const Options &opt);
  void start();
  void stop();

  int totalChunks() const { return mChunkDim[0]*mChunkDim[1]*mChunkDim[2]; }
  int centerChunks() const { return (mChunkDim[0]-2)*(mChunkDim[1]-2)*(mChunkDim[2]-2); }
  int numLoaded() const { return mNumLoaded; }
  int numMeshed();

  // rendering
  void initGL(QObject *qparent);
  void cleanupGL();
  void render(Matrix4 pvm);

  void setDebug(bool debug) { mDebug = debug; }
  void setFluidSim(bool on) { mSimFluids = on; }
  void setFluidEvap(bool on) { mEvapFluids = on; }
  void clearFluids()
  {
    LOGDC(COLOR_YELLOW, "CLEARING FLUIDS!!");
    mFluids.clear();
    std::lock_guard<std::mutex> lock(mRenderLock);
    mFluidMeshes.clear();
  }

  void setFrustum(Frustum *frustum);
  void setFrustumClip(bool on);
  void setFrustumPause();

  // updating
  void update();
  void step();
  bool readyForPlayer() const;
  Point3f getStartPos(const Point3i &pPos);
  
  void setLightLevel(int level) { mLighting = level; }
  void setCenter(const Point3i &chunkCenter);
  void setRadius(const Vector3i &chunkRadius);
  Point3i getCenter() const;
  Vector3i getRadius() const { return mLoadRadius; }
  void setCamPos(const Point3f &pos) { mCamPos = pos; }

  block_t* at(const Point3i &wp);
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
  bool mClipFrustum = true;

  Frustum *mFrustum;
  std::unordered_map<int32_t, bool> mFrustumRender;
  
  float mFogStart;
  float mFogEnd;
  Vector3f mDirScale;
  bool mRadChanged = false;
  
  ChunkLoader *mLoader;
  Point3i mCenter;
  Vector3i mLoadRadius;
  Vector3i mChunkDim;
  Point3i mMinChunk;
  Point3i mMaxChunk;
  
  int mLighting = 0;
  Point3f mCamPos;

  
  struct MeshedChunk
  {
    int32_t hash = 0;
    MeshData mesh;
  };
  
  FluidManager mFluids;
  ThreadPool mMeshPool;
  centerLoopCallback_t mCenterLoopCallback;
  
  // chunk updating / threading
  std::mutex mChunkLock;
  std::mutex mMeshedLock;
  std::mutex mUnloadLock;
  std::mutex mRenderLock;
  std::mutex mRenderQueueLock;
  std::mutex mFRenderQueueLock;
  std::mutex mUnusedMCLock;
  std::mutex mUnloadMeshLock;
  std::mutex mLoadLock;
  std::mutex mBoundaryLock;
  std::mutex mNeighborLock;
  std::mutex mMeshLock;
  std::condition_variable mMeshCv;
    
  int mMaxLoad;
  std::atomic<int> mNumLoading;
  int mCenterDistIndex = 0;
  std::atomic<int> mNumLoaded = 0;

  // active maps
  std::unordered_map<int32_t, Chunk*> mChunks;
  std::unordered_map<int32_t, std::unordered_map<blockSide_t, Chunk*>> mNeighbors;
  std::unordered_map<int32_t, OuterShell*> mChunkBoundaries;
  std::unordered_map<int32_t, ChunkMesh*> mRenderMeshes;
  std::unordered_map<int32_t, ChunkMesh*> mFluidMeshes;

  // resource cycling
  std::queue<Chunk*> mUnusedChunks;
  std::queue<ChunkMesh*> mUnusedMeshes;
  std::queue<MeshedChunk*> mUnusedMC;

  // queues
  std::queue<MeshedChunk*> mRenderQueue;
  std::queue<MeshedChunk*> mFRenderQueue;
  std::unordered_set<int32_t> mUnloadMeshes;
  std::vector<Chunk*> mMeshQueue;
  std::queue<Chunk*> mLoadQueue;

  // for keeping track
  std::unordered_set<int32_t> mMeshing;
  std::unordered_set<int32_t> mMeshed;

  
  // rendering
  Shader *mBlockShader = nullptr;
  Shader *mChunkLineShader = nullptr;
  cTextureAtlas *mTexAtlas = nullptr;
  cMeshBuffer* mChunkLineMesh = nullptr;


  bool isEdge(int32_t hash);
  blockSide_t getEdges(int32_t hash);
  void meshChunk(Chunk *chunk);
  
  block_t* atBlock(const Point3i &wp, std::unordered_map<int32_t, Chunk*> &neighbors);
  block_t getBlock(const Point3i &wp, std::unordered_map<int32_t, Chunk*> &neighbors);
  void updateAdjacent(const Point3i &wp);

  bool neighborsLoaded(int32_t hash, Chunk *chunk);
  void chunkLoadCallback(Chunk *chunk);
  bool checkChunkLoad(const Point3i &cp);

  void meshWorker(int tid);
  int getAO(int e1, int e2, int c);
  int getLighting(const Point3i &bp, const Point3f &vpos, blockSide_t side,
                  std::unordered_map<int32_t, Chunk*> &neighbors );
  void updateChunkMesh(Chunk *chunk);
  void addMesh(MeshedChunk *mc);

  MeshData makeChunkLineMesh();

  bool updateInfo();
  //bool correctChunk(const Point3i &bp, const Point3i &bi, Chunk *chunk, Chunk* &chunkOut, Point3i &pOut);
  //bool updateChunkEdges(Chunk *chunk);

};

#endif // WORLD_HPP
