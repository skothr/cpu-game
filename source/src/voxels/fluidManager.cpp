#include "fluidManager.hpp"

#include "block.hpp"
#include "fluid.hpp"
#include "fluidChunk.hpp"
#include "meshing.hpp"
#include "meshData.hpp"
#include "world.hpp"
#include "pointMath.hpp"

#include <unordered_set>

FluidManager::FluidManager(){}
FluidManager::~FluidManager()
{
  mFluids.lock();
  for(auto iter : mFluids)
    {
      if(iter.second)
        { delete iter.second; }
    }
  mFluids.unlock();
  mFluids.clear();
  mMeshLock.lock();
  for(auto iter : mMeshData)
    {
      delete iter.second;
    }
  mMeshData.clear();
  mMeshLock.unlock();
  mBoundaries.lock();
  for(auto iter : mBoundaries)
    {
      if(iter.second)
        { delete iter.second; }
    }
  mBoundaries.unlock();
  mBoundaries.clear();
}

void FluidManager::clear()
{
  mFluids.clear();
  std::lock_guard<std::mutex> lock(mMeshLock);
  mMeshData.clear();
}

void FluidManager::setRange(const Point3i &min, const Point3i &max)
{
  mMin = min;
  mMax = max;
}

bool FluidManager::makeFluidChunk(int32_t hash, const std::unordered_map<int32_t, Fluid*> &fluids)
{
  if(fluids.size() > 0 && pointInRange(Hash::unhash(hash), mMin, mMax))
    {
      FluidChunk *fluid = new FluidChunk(Hash::unhash(hash), fluids);
      if(mFluids.contains(hash))
        { delete mFluids[hash]; }
      mFluids.emplace(hash, fluid);
      fluid->setDirty(true);
      fluid->setIncomplete(true);
      return true;
    }
  return false;
}
bool FluidManager::set(const Point3i &wp, Fluid *fluid)
{
  hash_t cHash = Hash::hash(World::chunkPos(wp));
  FluidChunk *chunk = mFluids[cHash];
  if(chunk)
    {
      chunk->set(Chunk::blockPos(wp), fluid);
      chunk->setDirty(true);
      chunk->setIncomplete(true);
      return true;
    }
  else
    { return makeFluidChunk(cHash, {{Hash::hash(Chunk::blockPos(wp)), fluid}}); }
}



bool FluidManager::makeFluidChunkLocked(int32_t hash, const std::unordered_map<int32_t, Fluid*> &fluids)
{
  if(fluids.size() > 0)
    {
      FluidChunk *fluid = new FluidChunk(Hash::unhash(hash), fluids);
      if(mFluids.lockedContains(hash))
        { delete mFluids.lockedAt(hash); }
      mFluids.lockedEmplace(hash, fluid);
      fluid->setDirty(true);
      fluid->setIncomplete(true);
      return true;
    }
  return false;
}
bool FluidManager::setLocked(const Point3i &wp, Fluid *fluid)
{
  hash_t cHash = Hash::hash(World::chunkPos(wp));
  FluidChunk *chunk = mFluids.lockedAt(cHash);
  if(chunk)
    {
      chunk->set(Chunk::blockPos(wp), fluid);
      chunk->setDirty(true);
      chunk->setIncomplete(true);
      return true;
    }
  else
    {
      return makeFluidChunkLocked(cHash, {{Hash::hash(Chunk::blockPos(wp)), fluid}});
    }
}

int FluidManager::numBlocks()
{
  int num = 0;
  mFluids.lock();
  for(auto iter : mFluids)
    { num += iter.second->numBlocks(); }
  mFluids.unlock();
  return num;
}
int FluidManager::numChunks() const
{ return mFluids.size(); }

bool FluidManager::setChunkBoundary(int32_t hash, OuterShell *boundary)
{
  mBoundaries.emplace(hash, boundary);
  return mFluids.contains(hash);
}
bool FluidManager::setChunk(int32_t hash, Chunk *chunk)
{
  mChunks.emplace(hash, chunk);
  //mFluids[hash]->setDirty(true);
  return true;
}

std::unordered_map<int32_t, MeshData*> FluidManager::getUpdates()
{
  std::unordered_map<int32_t, MeshData*> ready;
  {
    std::lock_guard<std::mutex> lock(mMeshLock);
    ready = mMeshData;
    mMeshData.clear();
  }
  return ready;
}

static const std::array<blockSide_t, 6> sides {{ blockSide_t::PX, blockSide_t::PY,
                                                    blockSide_t::PZ, blockSide_t::NX,
                                                    blockSide_t::NY, blockSide_t::NZ }};
static const std::array<Point3i, 6> sideDirections {{ Point3i{1,0,0}, Point3i{0,1,0},
                                                      Point3i{0,0,1}, Point3i{-1,0,0},
                                                      Point3i{0,-1,0}, Point3i{0,0,-1} }};
static const std::array<int, 4> horizontalDirs {{0,1,3,4}};

std::unordered_map<int32_t, bool> FluidManager::step(bool evapFluids)
{
#define FLOW 0.1
#define ZFLOW 1.0

  mFluids.lock();
  std::unordered_map<int32_t, bool> updates;
  std::unordered_set<int32_t> emptyChunks;
  Fluid *newFluid = new Fluid(block_t::WATER, 1.0f, 1.0f);
  std::array<FluidChunk*, 6> nChunks;
  std::array<OuterShell*, 6> nBounds;
  std::array<Point3i, 6> nPos;
  std::array<Point3i, 6> ncPos;
  std::array<Point3i, 6> nbPos;
  const int pxi = 0;
  const int pyi = 1;
  const int pzi = 2;
  const int nxi = 3;
  const int nyi = 4;
  const int nzi = 5;
  for(auto iter : mFluids)
    {
      if(!pointInRange(iter.second->pos(), mMin, mMax))
        { continue; }
      std::unordered_set<int32_t> emptied;
      const hash_t cHash = iter.first;
      FluidChunk *chunk = iter.second;
      const Point3i cp = chunk->pos();
      bool chunkChanged = false;
      std::array<ActiveBlock*, 6> nBlock = {{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}};
      for(auto &fIter : chunk->data())
        {
          const int bHash = fIter.first;
          Fluid *fluid = fIter.second;
          if(fluid->isEmpty())
            {
              emptied.insert(bHash);
              continue;
            }

          *newFluid = *fluid;
          const Point3i bp = Hash::unhash(bHash);
          const Point3i wp = cp*Chunk::size + bp;
          for(int i = 0; i < 6; i++)
            {
              nPos[i] = wp + sideDirections[i];
              ncPos[i] = World::chunkPos(nPos[i]);
              nbPos[i] = Chunk::blockPos(nPos[i]);
              nChunks[i] = getFluidChunkLocked(nPos[i]);
              if(!nChunks[i])
                {
                  setLocked(nPos[i], nullptr);
                  nChunks[i] = getFluidChunkLocked(nPos[i]);
                }
              nBounds[i] = getBoundary(nPos[i]);
            }
          ActiveBlock *bNZ = nullptr;
          bool fluidFull = true;
          if(nBounds[nzi])
            {
              nBounds[nzi]->lock();
              bNZ = nBounds[nzi]->getBlock(Hash::hash(nbPos[nzi]));
              nBounds[nzi]->unlock();
            }
          if(!bNZ || !(bool)(bNZ->sides & blockSide_t::PZ) || bNZ->block == block_t::NONE)
            {
              Fluid *nzFluid = nChunks[nzi]->at(nbPos[nzi]);
              if(!nzFluid)
                {
                  nChunks[nzi]->set(nbPos[nzi], newFluid);
                  fIter.second->fluidLevel = 0.0f;
                  emptied.insert(bHash);
                  updates[cHash] = true;
                  updates[Hash::hash(ncPos[nzi])] = true;
                }
              else
                {
                  float overflow = nzFluid->adjustLevel(fluid->fluidLevel);
                  fluid->fluidLevel = overflow;
                  if(overflow > 0.0f)
                    { fluidFull = true; }
                  updates[cHash] = true;
                  updates[Hash::hash(ncPos[nzi])] = true;
                }
            }

          if((bNZ && isSimpleBlock(bNZ->block)) || fluidFull)
            { // spread out in each direction
              for(auto s : horizontalDirs)
                {
                  ActiveBlock *b = nullptr;
                  Fluid *f = nullptr;
                  if(nBounds[s])
                    {
                      nBounds[s]->lock();
                      b = nBounds[s]->getBlock(Hash::hash(nbPos[s]));
                      nBounds[s]->unlock();
                    }
                  if(nChunks[s])
                    { f = nChunks[s]->at(nbPos[s]); }

                  if(!b || !(bool)(b->sides & oppositeSide(sides[s])))
                    { // side is open

                      float pressure = fluid->fluidLevel - (f ? (f->fluidLevel) : 0);

                      float flowFull = 0.2f;
                      float flowEmpty = 0.2f;
                      if(pressure > 0)
                        { // apply pressure
                          if(!f)
                            { // add fluid cell
                              newFluid->fluidLevel = 0.0f;
                              setLocked(nPos[s], newFluid);
                              f = getFluidChunkLocked(nPos[s])->at(nbPos[s]);
                            }
                          float flow = lerp(flowEmpty, flowFull, 1.0f - pressure);
                          float overflow = f->adjustLevel(pressure * flow);
                          fluid->adjustLevel(-pressure * flow + overflow);
                          updates[cHash] = true;
                          updates[Hash::hash(ncPos[s])] = true;
                          if(f->isEmpty())
                            { emptied.insert(Hash::hash(nbPos[s])); }
                        }
                    }
                }
            }
        }
      for(auto f : emptied)
        { chunk->set(Hash::unhash(f), nullptr); }
      if(chunk->step(evapFluids))
        { updates[cHash] = true; }
      if(chunk->isEmpty())
        { emptyChunks.insert(cHash); }
    }

  delete newFluid;
  mFluids.unlock();
  
  for(auto &c : emptyChunks)
    {
      updates[c] = true;
      mFluids.erase(c);
      
      std::lock_guard<std::mutex> lock(mMeshLock);
      auto iter = mMeshData.find(c);
      if(iter != mMeshData.end())
        { delete iter->second; }
      mMeshData[c] = nullptr;
    }
  for(auto iter : updates)
    {
      if(iter.second && emptyChunks.count(iter.first) == 0)
        { makeMesh(iter.first); }
    }
  return updates;
}

FluidChunk* FluidManager::getFluidChunk(const Point3i &wp)
{
  return mFluids[Hash::hash(World::chunkPos(wp))];
}
FluidChunk* FluidManager::getFluidChunkLocked(const Point3i &wp)
{
  return mFluids.lockedAt(Hash::hash(World::chunkPos(wp)));
}
OuterShell* FluidManager::getBoundary(const Point3i &wp)
{
  return mBoundaries[Hash::hash(World::chunkPos(wp))];
}

// CUBE FACE INDICES
static const std::array<unsigned int, 6> faceIndices = { 0, 1, 2, 3, 1, 0 };
static const std::array<unsigned int, 6> reverseIndices = { 0, 1, 3, 2, 1, 0 };
static const std::array<unsigned int, 6> flippedIndices = { 0, 3, 2, 1, 2, 3 };
static std::unordered_map<blockSide_t, std::array<cSimpleVertex, 4>> faceVertices
  { // CUBE FACE VERTICES
   {blockSide_t::PX,
    {cSimpleVertex(Point3f{1, 0, 0}, Vector3f{1, 0, 0}, Vector2f{0.0f, 0.0f} ),
     cSimpleVertex(Point3f{1, 1, 1}, Vector3f{1, 0, 0}, Vector2f{1.0f, 1.0f} ),
     cSimpleVertex(Point3f{1, 0, 1}, Vector3f{1, 0, 0}, Vector2f{0.0f, 1.0f} ),
     cSimpleVertex(Point3f{1, 1, 0}, Vector3f{1, 0, 0}, Vector2f{1.0f, 0.0f} ) } },
   {blockSide_t::PY,
    {cSimpleVertex(Point3f{0, 1, 0}, Vector3f{0, 1, 0}, Vector2f{0.0f, 0.0f} ),
     cSimpleVertex(Point3f{1, 1, 1}, Vector3f{0, 1, 0}, Vector2f{1.0f, 1.0f} ),
     cSimpleVertex(Point3f{1, 1, 0}, Vector3f{0, 1, 0}, Vector2f{0.0f, 1.0f} ),
     cSimpleVertex(Point3f{0, 1, 1}, Vector3f{0, 1, 0}, Vector2f{1.0f, 0.0f} ) } },
   {blockSide_t::PZ,
    {cSimpleVertex(Point3f{0, 0, 1}, Vector3f{0, 0, 1}, Vector2f{0.0f, 0.0f} ),
     cSimpleVertex(Point3f{1, 1, 1}, Vector3f{0, 0, 1}, Vector2f{1.0f, 1.0f} ),
     cSimpleVertex(Point3f{0, 1, 1}, Vector3f{0, 0, 1}, Vector2f{0.0f, 1.0f} ),
     cSimpleVertex(Point3f{1, 0, 1}, Vector3f{0, 0, 1}, Vector2f{1.0f, 0.0f} ) } },
   {blockSide_t::NX,
    {cSimpleVertex(Point3f{0, 0, 0}, Vector3f{-1, 0, 0}, Vector2f{0.0f, 0.0f} ),
     cSimpleVertex(Point3f{0, 1, 1}, Vector3f{-1, 0, 0}, Vector2f{1.0f, 1.0f} ),
     cSimpleVertex(Point3f{0, 1, 0}, Vector3f{-1, 0, 0}, Vector2f{0.0f, 1.0f} ),
     cSimpleVertex(Point3f{0, 0, 1}, Vector3f{-1, 0, 0}, Vector2f{1.0f, 0.0f} ) } },
   {blockSide_t::NY,
    {cSimpleVertex(Point3f{0, 0, 0}, Vector3f{0, -1, 0}, Vector2f{0.0f, 0.0f} ),
     cSimpleVertex(Point3f{1, 0, 1}, Vector3f{0, -1, 0}, Vector2f{1.0f, 1.0f} ),
     cSimpleVertex(Point3f{0, 0, 1}, Vector3f{0, -1, 0}, Vector2f{0.0f, 1.0f} ),
     cSimpleVertex(Point3f{1, 0, 0}, Vector3f{0, -1, 0}, Vector2f{1.0f, 0.0f} ) } },
   {blockSide_t::NZ,
    {cSimpleVertex(Point3f{0, 0, 0}, Vector3f{0, 0, -1}, Vector2f{0.0f, 0.0f} ),
     cSimpleVertex(Point3f{1, 1, 0}, Vector3f{0, 0, -1}, Vector2f{1.0f, 1.0f} ),
     cSimpleVertex(Point3f{1, 0, 0}, Vector3f{0, 0, -1}, Vector2f{0.0f, 1.0f} ),
     cSimpleVertex(Point3f{0, 1, 0}, Vector3f{0, 0, -1}, Vector2f{1.0f, 0.0f} ) } } };


void FluidManager::makeMesh(int32_t hash)
{
  FluidChunk *chunk = mFluids[hash];
  if(!chunk)
    { return; }
  
  Point3i cp = chunk->pos();
  Point3i minP = cp*Chunk::size;
  
  if(chunk)
    {
      MeshData *mesh = new MeshData();
      for(auto cIter : chunk->data())
        {
          int32_t cHash = cIter.first;
          Point3i bp = Hash::unhash(cHash);
          Fluid *fluid = cIter.second;
          if(!fluid->isEmpty())
            {             
              const Point3f vOffset = minP + bp;
              for(int i = 0; i < 6; i++)
                {
                  const unsigned int numVert = mesh->vertices().size();
                  for(auto &v : faceVertices[sides[i]])
                    { // add vertices for this face
                      Point3f vp = v.pos;
                      if(vp[2] != 0)
                        { vp[2] *= fluid->fluidLevel; }
                      mesh->vertices().emplace_back(vOffset + vp,
                                                    v.normal,
                                                    v.texcoord,
                                                    (int)fluid->type - 1,
                                                    (float)4 / (float)4 );
                    }
                  for(auto i : faceIndices)
                    { mesh->indices().push_back(numVert + i); }
                }
            }
        }
      std::lock_guard<std::mutex> lock(mMeshLock);
      auto iter = mMeshData.find(hash);
      if(iter != mMeshData.end())
        {
          delete iter->second;
          iter->second = mesh;
        }
      else
        {
          mMeshData.emplace(hash, mesh);
        }
    }
}
