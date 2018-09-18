#ifndef MESH_RENDERER_HPP
#define MESH_RENDERER_HPP

#include "block.hpp"
#include "hashing.hpp"
#include "vector.hpp"
#include "threadPool.hpp"
#include "meshData.hpp"
#include "geometry.hpp"

#include <queue>
#include <deque>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <condition_variable>

class QObject;
class Chunk;
class cTextureAtlas;
class Shader;
class Frustum;
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

  void start();
  void stop();
  void setMeshThreads(int meshThreads);

  bool initGL(QObject *qParent);
  void cleanupGL();
  void render(const Matrix4 &pvm, const Point3f &camPos);
  void setFrustum(Frustum *frustum);
  void setFrustumClip(bool on);
  void setFrustumPause();

  void setFluids(FluidManager *fluids)
  { mFluids = fluids; }
  void clearFluids()
  {
    std::lock_guard<std::mutex> lock(mRenderLock);
    mFluidMeshes.clear();
  }
  int numMeshed();

  bool isMeshed(hash_t hash);

  void load(Chunk *chunk, const Point3i &center);
  void reorderQueue(const Point3i &newCenter);
  void unload(hash_t hash);

  void setFog(float fogStart, float fogEnd, const Vector3f &dirScale);
  void setCenter(const Point3i &pos) { mCenter = pos; }
  
private:
  // CUBE FACE INDICES
  static const std::array<unsigned int, 6> faceIndices;
  static const std::array<unsigned int, 6> reverseIndices;
  static const std::array<unsigned int, 6> flippedIndices;
  static std::unordered_map<blockSide_t, std::array<cSimpleVertex, 4>> faceVertices;
  static const std::array<blockSide_t, 6> meshSides;
  static const std::array<Point3i, 6> meshSideDirections;

  bool mInitialized = false;
  bool mClipFrustum = true;
  Frustum *mFrustum = nullptr;
  std::unordered_set<hash_t> mFrustumRender;

  FluidManager *mFluids = nullptr;
  
  Shader *mBlockShader = nullptr;
  cTextureAtlas *mTexAtlas = nullptr;

  ThreadPool mMeshPool;

  bool mFogChanged = false;
  float mFogStart = 0.0f;
  float mFogEnd = 0.0f;
  Vector3f mDirScale;

  Point3i mCenter;
  
  struct MeshedChunk
  { hash_t hash = 0;
    MeshData mesh; };
  
  std::mutex mMeshLock;
  std::condition_variable mMeshCv;
  std::deque<Chunk*> mLoadQueue;
  std::unordered_set<hash_t> mMeshing;
  
  std::mutex mUnloadLock;
  std::unordered_set<hash_t> mUnloadQueue;
  
  std::mutex mMeshedLock;
  std::unordered_set<hash_t> mMeshed;
  std::mutex mUnusedMCLock;
  std::queue<MeshedChunk*> mUnusedMC;
  
  std::mutex mRenderLock;
  std::unordered_map<hash_t, ChunkMesh*> mRenderMeshes;
  std::unordered_map<hash_t, ChunkMesh*> mFluidMeshes;
  std::queue<ChunkMesh*> mUnusedMeshes;
  
  std::mutex mRenderQueueLock;
  std::mutex mFRenderQueueLock;
  std::queue<MeshedChunk*> mRenderQueue;
  std::queue<MeshedChunk*> mFRenderQueue;
  
  Shader *mComplexShader = nullptr;
  std::unordered_map<block_t, ModelObj*> mComplexModels;
  std::unordered_map<hash_t, std::unordered_map<int, ComplexBlock*>> mComplexBlocks;

  std::mutex mTimingLock;
  double mMeshTime = 0.0;
  int mMeshNum = 0;
  double mBoundsTime = 0.0;
  int mBoundsNum = 0;
  
  void meshWorker(int tid);
  block_t getBlock(Chunk *chunk, const Point3i &wp);
  int getAO(int e1, int e2, int c);
  int getLighting(Chunk *chunk, const Point3i &wp, const Point3f &vp,
                  blockSide_t side );
  void updateChunkMesh(Chunk *chunk);
  void addMesh(MeshedChunk *mc);
};


#endif // MESH_RENDERER_HPP
