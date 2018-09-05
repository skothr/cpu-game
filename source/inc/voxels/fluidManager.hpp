#ifndef FLUID_MANAGER_HPP
#define FLUID_MANAGER_HPP

#include <unordered_map>
#include <mutex>
#include "threadMap.hpp"

class MeshData;
class ChunkMesh;
class OuterShell;
class Chunk;
class Fluid;
class FluidChunk;
class Chunk;

class FluidManager
{
public:
  FluidManager();
  ~FluidManager();

  bool makeFluidChunk(int32_t hash, const std::unordered_map<int32_t, Fluid*> &fluids);
  bool setChunkBoundary(int32_t hash, OuterShell *boundary);
  bool setChunk(int32_t hash, Chunk *chunk);
  std::unordered_map<int32_t, bool> step(bool evapFluids);
  void setRange(const Point3i &min, const Point3i &max);

  //bool uploadMesh(int32_t hash, ChunkMesh *mesh);
  //bool renderMeshes();
  std::unordered_map<int32_t, MeshData*> getUpdates();

  bool set(const Point3i &wp, Fluid *fluid);
  int numBlocks();
  int numChunks() const;
  void clear();
  
private:
  ThreadMap<FluidChunk*> mFluids;
  ThreadMap<Chunk*> mChunks;
  ThreadMap<OuterShell*> mBoundaries;
  std::mutex mMeshLock;
  std::unordered_map<int32_t, MeshData*> mMeshData;

  Point3i mMin;
  Point3i mMax;
  
  OuterShell* getBoundary(const Point3i &wp);
  FluidChunk* getFluidChunk(const Point3i &wp);
  void makeMesh(int32_t hash);
  
  bool makeFluidChunkLocked(int32_t hash, const std::unordered_map<int32_t, Fluid*> &fluids);
  FluidChunk* getFluidChunkLocked(const Point3i &wp);
  bool setLocked(const Point3i &wp, Fluid *fluid);  
};

#endif // FLUID_MANAGER_HPP
