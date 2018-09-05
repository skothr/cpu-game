#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

/*
#include <queue>
#include "block.hpp"


class ChunkManager
{
public:
  ChunkManager();
  ~ChunkManager();
  
  int totalChunks() const { return mChunkDim[0]*mChunkDim[1]*mChunkDim[2]; }
  int centerChunks() const { return (mChunkDim[0]-2)*(mChunkDim[1]-2)*(mChunkDim[2]-2); }
  int numLoaded() const { return mNumLoaded; }
  int numMeshed();
  
  // rendering
  void initGL(QObject *qparent);
  void cleanupGL();
  void render(Matrix4 pvm);

  // updating
  void update();
  //bool step();

  bool updateMesh(int32_t hash);
  
  void setCenter(const Point3i &chunkCenter);
  void setRadius(const Vector3i &chunkRadius);
  Point3i getCenter() const;
  Vector3i getRadius() const { return mLoadRadius; }

  block_t at(const Point3i &wp);
  Chunk* getChunk(const Point3i &wp);
  bool setBlock(const Point3i &p, block_t type);
  
private:
  // chunk updating / threading
  std::mutex mChunkLock;
  std::atomic<int> mNumLoaded = 0;
  std::mutex mMeshedLock;
  std::unordered_set<int32_t> mMeshed;
  std::mutex mUnloadLock;
  std::unordered_set<int32_t> mUnloadChunks;
  
  std::mutex mRenderLock;
  std::unordered_map<int32_t, ChunkMesh*> mRenderMeshes;
  std::queue<ChunkMesh*> mUnusedMeshes;
  std::mutex mRenderQueueLock;
  std::queue<MeshedChunk*> mRenderQueue;
  std::mutex mUnusedMCLock;
  std::queue<MeshedChunk*> mUnusedMC;
  std::mutex mUnloadMeshLock;
  std::unordered_set<int32_t> mUnloadMeshes;
  
  std::vector<Chunk*> mMeshQueue;
  std::unordered_set<int32_t> mMeshing;

  std::queue<Chunk*> mUnusedChunks;
  std::unordered_map<int32_t, Chunk*> mChunks;

};

*/
#endif // CHUNK_MANAGER_HPP
