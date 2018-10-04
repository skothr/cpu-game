#ifndef MESH_RENDERER_HPP
#define MESH_RENDERER_HPP

#include "block.hpp"
#include "camera.hpp"
#include "hashing.hpp"
#include "vector.hpp"
#include "threadPool.hpp"
#include "meshData.hpp"
#include "matrix.hpp"
#include "chunkMap.hpp"

#include <queue>
#include <deque>
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <condition_variable>

class QObject;
class Chunk;
class cTextureAtlas;
class Shader;
class ChunkMesh;
class FluidManager;
class ModelObj;

static std::unordered_map<block_t, std::string> gComplexModelPaths
  { {block_t::DEVICE, "./res/device.obj"},
    {block_t::MEMORY, "./res/memory.obj"},
    {block_t::CPU, "./res/cpu.obj"}};


class MeshRenderer
{
public:
  MeshRenderer();
  ~MeshRenderer();

  void startMeshing();
  void stopMeshing();
  void setMeshThreads(int meshThreads);
  void clearMeshes();
  
  bool initGL(QObject *qParent);
  void cleanupGL();
  void render(const Matrix4 &pvm, const Point3f &camPos, bool reset = false);
  void setCamera(Camera *camera);
  void setFrustumCulling(bool on);
  void pauseFrustumCulling();

  MeshData makeFrustumMesh();

  void setFluids(FluidManager *fluids)
  { mFluids = fluids; }
  void clearFluids()
  {
    std::lock_guard<std::mutex> lock(mRenderLock);
    mFluidMeshes.clear();
  }
  int numMeshed();

  bool isMeshed(hash_t hash);
  bool isMeshing(hash_t hash);
  bool isVisible(hash_t hash);

  void load(Chunk *chunk, const Point3i &center, bool priority);
  void reorderQueue(const Point3i &newCenter);
  void unload(hash_t hash);

  void setFog(float fogStart, float fogEnd, const Vector3f &dirScale);
  void setCenter(const Point3i &pos) { mCenter = pos; }
  void setRadius(const Vector3i &rad) { mRadius = rad; }

  void setMap(ChunkMap *map)
  { mMap = map; }

  struct TraverseLine
  {
    hash_t first;
    hash_t second;
    bool verified;
    int numSteps;
  };
  std::vector<TraverseLine> getRenderOrder()
  { std::lock_guard<std::mutex> lock(mRenderLock); return mRenderOrder; }

  
private:
  // CUBE FACE INDICES
  static const std::array<unsigned int, 6> faceIndices;
  static const std::array<unsigned int, 6> reverseIndices;
  static const std::array<unsigned int, 6> flippedIndices;
  static std::unordered_map<blockSide_t, std::array<cSimpleVertex, 4>> faceVertices;
  static const std::array<blockSide_t, 6> meshSides;
  static const std::array<Point3i, 6> meshSideDirections;

  bool mInitialized = false;
  bool mFrustumCulling = true;
  bool mFrustumPaused = false;
  Camera *mCamera = nullptr;
  Camera mPausedCamera;

  FluidManager *mFluids = nullptr;
  
  Shader *mBlockShader = nullptr;
  cTextureAtlas *mTexAtlas = nullptr;

  ThreadPool mMeshPool;
  std::atomic<int> mWaitingThreads = 0;

  bool mFogChanged = false;
  float mFogStart = 0.0f;
  float mFogEnd = 0.0f;
  Vector3f mDirScale;

  Vector3i mRadius;
  Point3i mCenter;
  
  struct MeshedChunk
  {
    hash_t hash = 0;
    MeshData mesh;
  };
  
  std::mutex mMeshLock;
  std::condition_variable mMeshCv;
  std::condition_variable mPriorityCv;
  std::list<Chunk*> mMeshQueue;
  std::list<Chunk*> mPriorityMeshQueue;
  std::unordered_set<hash_t> mMeshing;
  
  std::mutex mUnloadLock;
  std::unordered_set<hash_t> mUnloadQueue;
  
  std::mutex mMeshedLock;
  std::mutex mNowLock;
  std::unordered_set<hash_t> mMeshed;
  std::unordered_set<hash_t> mMeshingNow;
  std::mutex mUnusedMCLock;
  std::queue<MeshedChunk*> mUnusedMC;
  
  std::mutex mRenderLock;
  std::unordered_map<hash_t, ChunkMesh*> mRenderMeshes;
  std::unordered_map<hash_t, ChunkMesh*> mFluidMeshes;
  std::queue<ChunkMesh*> mUnusedMeshes;
  std::unordered_map<hash_t, Chunk*> mRenderChunks;
  std::mutex mChunkLock;

  std::mutex mRenderQueueLock;
  std::mutex mFRenderQueueLock;
  std::queue<MeshedChunk*> mRenderQueue;
  std::queue<MeshedChunk*> mFRenderQueue;
  
  Shader *mComplexShader = nullptr;
  Shader *mMiniMapShader = nullptr;
  std::unordered_map<block_t, ModelObj*> mComplexModels;
  std::unordered_map<hash_t, std::unordered_map<int, ComplexBlock*>> mComplexBlocks;

  std::vector<TraverseLine> mRenderOrder;
  std::unordered_set<hash_t> mVisible;

  std::mutex mTimingLock;
  double mMeshTime = 0.0;
  int mMeshNum = 0;
  double mBoundsTime = 0.0;
  int mBoundsNum = 0;

  ChunkMap *mMap;
  
  void meshWorker(int tid);
  block_t getBlock(Chunk *chunk, const Point3i &wp);
  int getAO(int e1, int e2, int c);
  int getLighting(Chunk *chunk, const Point3i &wp, const Point3f &vp,
                  blockSide_t side );
  void updateChunkMesh(Chunk *chunk);
  void addMesh(MeshedChunk *mc);
};


#endif // MESH_RENDERER_HPP
