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
      if(iter.second)
        { delete iter.second; }
    }
  mMeshData.clear();
  mMeshLock.unlock();
  
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

bool FluidManager::makeFluidChunk(int32_t hash)
{
  Point3i cp = Hash::unhash(hash);
  if(pointInRange(cp, mMin, mMax))
    {
      if(!mFluids.contains(hash))
        {
          mFluids.emplace(hash, new FluidChunk(cp));
        }
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
      return true;
    }
  else if(fluid)
    {
      if(makeFluidChunk(cHash))
        {
          mFluids[cHash]->set(Chunk::blockPos(wp), fluid);
          return true;
        }
      else
        { return false; }
    }
  else
    { return false; }
}

bool FluidManager::makeFluidChunkLocked(int32_t hash)
{
  Point3i cp = Hash::unhash(hash);
  if(pointInRange(cp, mMin, mMax))
    {
      if(!mFluids.lockedContains(hash))
        { mFluids.lockedEmplace(hash, new FluidChunk(cp)); }
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
      return true;
    }
  else
    {
      if(makeFluidChunkLocked(cHash))
        {
          mFluids.lockedAt(cHash)->set(Chunk::blockPos(wp), fluid);
        }
      else
        { return false; }
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

bool FluidManager::setChunkBoundary(int32_t hash, ChunkBounds *boundary)
{
  mBoundaries.emplace(hash, boundary);
  return mFluids.contains(hash);
}
bool FluidManager::setChunk(int32_t hash, Chunk *chunk)
{
  mChunks.emplace(hash, chunk);
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


bool updateFluid(Fluid *fluid, const Point3i &wp)
{
  

  
}


static const std::array<blockSide_t, 5> sides {{ blockSide_t::PX, blockSide_t::PY,
                                                    blockSide_t::NX, blockSide_t::NY,
                                                    blockSide_t::NZ }};
static const std::array<Point3i, 5> sideDirections {{ Point3i{1,0,0}, Point3i{0,1,0},
                                                      Point3i{-1,0,0}, Point3i{0,-1,0},
                                                      Point3i{0,0,-1} }};
static const std::array<int, 4> horizontalDirs {{0,1,2,3}};

std::unordered_map<int32_t, bool> FluidManager::step(float evapRate)
{
#define FLOW 0.1
#define ZFLOW 1.0

  mFluids.lock();
  std::unordered_map<int32_t, bool> updates;
  for(auto cIter : mFluids)
    {
      if(cIter.second->step(evapRate))
        { updates[cIter.first] = true; }
    }

  Fluid *newFluid = new Fluid();

  for(auto cIter : mFluids)
    {
      const hash_t cHash = cIter.first;
      FluidChunk *chunk = cIter.second;
      if(!pointInRange(chunk->pos(), mMin, mMax))
        { continue; }

      Point3i cOffset = chunk->pos() * FluidChunk::size;
      Point3i bp;
      for(bp[2] = 0; bp[2] < FluidChunk::sizeZ; bp[2]++)
        for(bp[0] = 0; bp[0] < FluidChunk::sizeX; bp[0]++)
          for(bp[1] = 0; bp[1] < FluidChunk::sizeY; bp[1]++)
            { // for each fluid block
              Fluid *fluid = chunk->at(bp);
              if(!fluid || fluid->isEmpty())
                { continue; }

              *newFluid = *fluid;
              
              Point3i wp = cOffset + bp;

              Point3i nzp = wp - Vector3i{0,0,1};
              Point3i nzbp = Chunk::blockPos(nzp);
              ChunkBounds *boundsNZ = getBoundary(nzp);
              FluidChunk *chunkNZ = getFluidChunkLocked(nzp);
              ActiveBlock *bNZ = nullptr;
              if(boundsNZ)
                {
                  boundsNZ->lock();
                  bNZ = boundsNZ->getBlock(Hash::hash(nzbp));
                  boundsNZ->unlock();
                }
              Point3i pzp = wp + Vector3i{0,0,1};
              Point3i pzbp = Chunk::blockPos(pzp);
              ChunkBounds *boundsPZ = getBoundary(pzp);
              FluidChunk *chunkPZ = getFluidChunkLocked(pzp);
              Fluid *pzFluid = nullptr;
              ActiveBlock *bPZ = nullptr;
              if(boundsPZ)
                {
                  boundsPZ->lock();
                  bPZ = boundsPZ->getBlock(Hash::hash(pzbp));
                  boundsPZ->unlock();
                }
              bool fluidFull = false;
              if(!bNZ)
                { // no block below this fluid cell
                  Fluid *nzFluid = chunkNZ ? chunkNZ->at(nzbp) : nullptr;
                  fluid->falling = true;

                  if(!nzFluid)
                    {
                      makeFluidChunkLocked(Hash::hash(World::chunkPos(nzp)));
                      chunkNZ = getFluidChunkLocked(nzp);
                      chunkNZ->set(nzbp, newFluid);
                      nzFluid = chunkNZ->at(nzbp);
                      nzFluid->level = 0.0f;
                      nzFluid->sideLevel = Vector<float, 4>();
                      nzFluid->nextLevel = 0.0f;
                      nzFluid->nextSideLevel = Vector<float, 4>();
                    }

                  float overflow = nzFluid->adjustNextLevel(fluid->level);
                  fluid->nextLevel = 0.0f;
                  if(overflow > 0.0f)
                    {
                      fluidFull = true;
                      if(!pzFluid)
                        {
                          makeFluidChunkLocked(Hash::hash(World::chunkPos(pzp)));
                          chunkPZ = getFluidChunkLocked(pzp);
                          chunkPZ->set(pzbp, newFluid);
                          pzFluid = chunkPZ->at(pzbp);
                          pzFluid->level = 0.0f;
                          pzFluid->sideLevel = Vector<float, 4>();
                          pzFluid->nextLevel = 0.0f;
                          pzFluid->nextSideLevel = Vector<float, 4>();
                        }
                      pzFluid->adjustNextLevel(overflow);
                    }
                  for(int i = 0; i < 4; i++)
                    {
                      overflow = nzFluid->adjustNextSideLevel(i, fluid->sideLevel[i]);
                      fluid->nextSideLevel[i] = 0.0f;
                      if(overflow > 0.0f)
                       {
                         fluidFull = true;
                         if(!pzFluid)
                           {
                             makeFluidChunkLocked(Hash::hash(World::chunkPos(pzp)));
                             chunkPZ = getFluidChunkLocked(pzp);
                             chunkPZ->set(pzbp, newFluid);
                             pzFluid = chunkPZ->at(pzbp);
                             pzFluid->level = 0.0f;
                             pzFluid->sideLevel = Vector<float, 4>();
                             pzFluid->nextLevel = 0.0f;
                             pzFluid->nextSideLevel = Vector<float, 4>();
                           }
                         pzFluid->adjustNextLevel(overflow);
                       }
                    }
                }

              if(bNZ || fluidFull)
                { // block or full fluid below.
                  fluid->falling = false;
                  
                  for(int s = 0; s < 4; s++)
                    {
                      Point3i sp = wp + sideDirections[s];
                      Point3i sbp = Chunk::blockPos(sp);
                      ChunkBounds *sbounds = getBoundary(sp);
                      FluidChunk *schunk = getFluidChunkLocked(sp);
                      if(!schunk)
                        {
                          makeFluidChunkLocked(Hash::hash(World::chunkPos(sp)));
                          schunk = getFluidChunkLocked(sp);
                        }
                      Fluid *sfluid = schunk->at(sbp);
                      if(!sfluid)
                        {
                          schunk->set(sbp, newFluid);
                          sfluid = schunk->at(sbp);
                          sfluid->level = 0.0f;
                          sfluid->sideLevel = Vector<float, 4>();
                          sfluid->nextLevel = 0.0f;
                          sfluid->nextSideLevel = Vector<float, 4>();
                        }
                      ActiveBlock *sblock = nullptr;
                      if(sbounds)
                        {
                          sbounds->lock();
                          sblock = sbounds->getBlock(Hash::hash(sbp));
                          sbounds->unlock();
                        }

                      if(!sblock)
                        { // side is open
                          float pressure = fluid->level - (sfluid ? (sfluid->level): 0);
                          const float flow = 0.1f;
                          if(pressure > 0.0f)
                            { // apply pressure
                              Point3i snzp = sp - Vector3i{0,0,1};
                              Point3i snzbp = Chunk::blockPos(snzp);
                              ChunkBounds *snzbounds = getBoundary(snzp);
                              FluidChunk *snzchunk = getFluidChunkLocked(snzp);
                              if(!snzchunk)
                                {
                                  makeFluidChunkLocked(Hash::hash(World::chunkPos(snzp)));
                                  snzchunk = getFluidChunkLocked(snzp);
                                }
                              Fluid *snzfluid = snzchunk->at(snzbp);
                              if(!snzfluid)
                                {
                                  snzchunk->set(snzbp, newFluid);
                                  snzfluid = snzchunk->at(snzbp);
                                  snzfluid->level = 0.0f;
                                  snzfluid->sideLevel = Vector<float, 4>();
                                  snzfluid->nextLevel = 0.0f;
                                  snzfluid->nextSideLevel = Vector<float, 4>();
                                }
                              ActiveBlock *snzblock = nullptr;
                              if(snzbounds)
                                {
                                  snzbounds->lock();
                                  snzblock = snzbounds->getBlock(Hash::hash(snzbp));
                                  snzbounds->unlock();
                                }
                              if(!snzblock)
                                {
                                  float overflow = snzfluid->adjustNextSideLevel((s+2)%4, pressure*flow);
                                  fluid->adjustNextLevel(-(pressure * flow) + overflow);
                                  updates[Hash::hash(World::chunkPos(snzp))] = true;
                                }
                              else
                                {
                                  float overflow = sfluid->adjustNextLevel(pressure*flow);
                                  fluid->adjustNextLevel(-(pressure * flow) + overflow);
                                  updates[Hash::hash(World::chunkPos(sp))] = true;
                                }
                              updates[Hash::hash(World::chunkPos(wp))] = true;
                            }
                        }
                    }
                }
            }
    }

  delete newFluid;
  mFluids.unlock();
  
  std::unordered_set<int32_t> emptyChunks;
  for(auto &cIter : mFluids)
    {
      hash_t cHash = cIter.first;
      FluidChunk *chunk = cIter.second;
      Point3i bp;
      for(bp[0] = 0; bp[0] < FluidChunk::sizeX; bp[0]++)
        for(bp[1] = 0; bp[1] < FluidChunk::sizeY; bp[1]++)
          for(bp[2] = 0; bp[2] < FluidChunk::sizeZ; bp[2]++)
            { // for each fluid block
              Fluid *fluid = chunk->at(bp);
              if(fluid)
                {
                  fluid->level = fluid->nextLevel;
                  fluid->sideLevel = fluid->nextSideLevel;
                  
                  if(fluid->isEmpty())
                    { chunk->set(bp, nullptr); }
                }
            }
      if(chunk->isEmpty())
        { emptyChunks.insert(cHash); }
    }
  for(auto &c : emptyChunks)
    {
      mFluids.lockedErase(c);
      std::lock_guard<std::mutex> lock(mMeshLock);
      auto iter = mMeshData.find(c);
      if(iter != mMeshData.end())
        { delete iter->second; }
      mMeshData[c] = nullptr;
    }
    
  for(auto iter : updates)
    {
      if(iter.second)
        { makeMesh(iter.first); }
    }
  
  for(auto &cIter : mFluids)
    {
      hash_t cHash = cIter.first;
      FluidChunk *chunk = cIter.second;
      Point3i bp;
      for(bp[0] = 0; bp[0] < FluidChunk::sizeX; bp[0]++)
        for(bp[1] = 0; bp[1] < FluidChunk::sizeY; bp[1]++)
          for(bp[2] = 0; bp[2] < FluidChunk::sizeZ; bp[2]++)
            { // for each fluid block
              Fluid *fluid = chunk->at(bp);
              if(fluid && !fluid->falling)
                {
                  for(int i = 0; i < 4; i++)
                    {
                      fluid->level += fluid->sideLevel[i];
                      fluid->sideLevel[i] = 0.0f;
                      fluid->nextSideLevel[i] = 0.0f;
                    }
                  fluid->nextLevel = fluid->level;
                }
            }
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
ChunkBounds* FluidManager::getBoundary(const Point3i &wp)
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



struct FaceRect
{
  FaceRect(blockSide_t ddir, const Point3f &pp1,const Point3f &pp2)
    : dir(ddir), p1(pp1), p2(pp2)
  { }
  blockSide_t dir;
  Point3f p1; 
  Point3f p2;
};



std::vector<Vector3f> normals =
  {Vector3f{1,0,0},   // PX
   Vector3f{0,1,0},   // PY
   Vector3f{-1,0,0},  // NX
   Vector3f{0,-1,0}}; // NY
std::vector<Vector3f> startPos =
  {Vector3f{1,0,0},   // PX
   Vector3f{1,1,0},   // PY
   Vector3f{0,1,0},   // NX
   Vector3f{0,0,0}};  // NY
std::vector<blockSide_t> blockSides =
  {blockSide_t::PX,   // PX
   blockSide_t::PY,   // PY
   blockSide_t::NX,   // NX
   blockSide_t::NY};  // NY
std::vector<FaceRect> getFluidRects(Fluid *fluid)
{
  std::vector<FaceRect> rects;
  //LOGD("%f", fluid->level);

  if(fluid->level > 0.0f)
    { // bottom rect
      rects.emplace_back(blockSide_t::NZ, Point3i{0,0,0}, Point3i{1,1,0});

      // level sides
      for(int i = 0; i < 4; i++)
        {
          const int left = (i+1)%4;
          const int opposite = (i+2)%4;
          const int right = (i+3)%4;
          const float sLevel = fluid->sideLevel[i];
          const float rLevel = fluid->sideLevel[right];
          const float lLevel = fluid->sideLevel[left];
          if(fluid->sideLevel[i] == 0.0f)
            {
              FaceRect r{blockSides[i], startPos[i],
                         startPos[i] + normals[left] + Vector3f{0,0,fluid->level}};
              r.p1 += normals[left] * rLevel;
              r.p2 += normals[right] * lLevel;
              rects.emplace_back(r);
            }
        }
      //level top
      FaceRect r({blockSide_t::PZ,
                  Vector3f{0,0,fluid->level},
                  Vector3f{1,1,fluid->level}});

      r.p1 += normals[0]*fluid->sideLevel[2] + normals[1]*fluid->sideLevel[3];
      r.p2 += normals[2]*fluid->sideLevel[0] + normals[3]*fluid->sideLevel[1];
      rects.push_back(r);
    }

  for(int i = 0; i < 4; i++)
    {
      const int left = (i+1)%4;
      const int opposite = (i+2)%4;
      const int right = (i+3)%4;
      const Vector3f base = startPos[i];
      const float sLevel = fluid->sideLevel[i];

      if(fluid->sideLevel[i] > 0.0f)
      {
        // side outer face
        rects.emplace_back(blockSides[i], base, base + normals[left] + Vector3f{0,0,1});

        // side inner face
        FaceRect ri {blockSides[opposite], base, base + normals[left]};
        // adjust points away from side face
        ri.p1 -= normals[i] * sLevel;
        ri.p2 -= normals[i] * sLevel;
        // adjust p1 updward due to fluid level
        ri.p1[2] += fluid->level;
        // adjust p1 inward due to right side level
        ri.p1 += normals[left] * fluid->sideLevel[right];
        // extends to top
        ri.p2[2]  = 1.0f;
        // adjust p2 inward due to left side level
        ri.p2 += normals[right] * fluid->sideLevel[left];
        rects.push_back(ri);

        // side top face
        FaceRect rt {blockSide_t::PZ, base + normals[opposite]*fluid->sideLevel[i],
                     base + normals[left]};
        rt.p1[2] = 1.0f; // both points on top
        rt.p2[2] = 1.0f;
      
        if((i % 2 == 0))
          { // only mesh top corners on two sides
            rt.p1 += normals[left] * fluid->sideLevel[right];
            rt.p2 += normals[right] * fluid->sideLevel[left];
          }
        rects.push_back(rt);

        //side outer side faces
        if(fluid->sideLevel[right] == 0.0f)
          {
            FaceRect rr {blockSides[right], base + normals[opposite]*fluid->sideLevel[i],
                         base + Vector3i{0,0,1}};
            rects.push_back(rr);
          }
        if(fluid->sideLevel[left] == 0.0f)
          {
            FaceRect rl {blockSides[left], base + normals[opposite]*fluid->sideLevel[i] + normals[left],
                         base + normals[left] + Vector3i{0,0,1}};
            rects.push_back(rl);
          }
      }
    }
  return rects;
}

void FluidManager::makeMesh(int32_t hash)
{
  FluidChunk *chunk = mFluids[hash];
  ChunkBounds *bounds = mBoundaries[hash];
  if(!chunk || !bounds)
    { return; }
  
  bounds->lock();
  auto shell = bounds->getBounds();
  bounds->unlock();
  
  Point3i cp = chunk->pos();
  Point3i minP = cp*Chunk::size;
  MeshData *mesh = new MeshData();
  Point3i bp;
  for(bp[0] = 0; bp[0] < FluidChunk::sizeX; bp[0]++)
    for(bp[1] = 0; bp[1] < FluidChunk::sizeY; bp[1]++)
      for(bp[2] = 0; bp[2] < FluidChunk::sizeZ; bp[2]++)
        {
          Fluid *fluid = chunk->at(bp);
          if(fluid && !fluid->isEmpty())
            {
              const Point3f vOffset = minP + bp;
              std::vector<FaceRect> rects = getFluidRects(fluid);
              //LOGD("RECTS: %d", rects.size());
              
              for(auto r : rects)
                {
                  Vector3i sdir = sideDirections[(int)r.dir];
                  int dim = sideDim(r.dir);
                  auto iter = shell.find(Hash::hash(bp + sdir));
                  bool draw = true;
                  for(auto &v : faceVertices[r.dir])
                    { // add vertices for this face
                      if(std::abs(v.pos[dim] + sdir[dim]*0.0001) > 1.0f)
                        {
                          draw = false;
                          break;
                        }
                    }

                  if(!draw && iter != shell.end() &&
                     iter->second.block != block_t::NONE &&
                     (iter->second.sides & oppositeSide(r.dir)) != blockSide_t::NONE)
                    { continue; }
                      
                  const unsigned int numVert = mesh->vertices().size();
                  for(auto &v : faceVertices[r.dir])
                    { // add vertices for this face
                      Point3f vp = v.pos;
                      vp[0] = (vp[0] == 0 ? std::min(r.p1[0], r.p2[0]) : std::max(r.p1[0], r.p2[0]));
                      vp[1] = (vp[1] == 0 ? std::min(r.p1[1], r.p2[1]) : std::max(r.p1[1], r.p2[1]));
                      vp[2] = (vp[2] == 0 ? std::min(r.p1[2], r.p2[2]) : std::max(r.p1[2], r.p2[2]));
                      mesh->vertices().emplace_back(vOffset + vp,
                                                    v.normal,
                                                    v.texcoord,
                                                    (int)fluid->type - 1,
                                                    (float)3 / (float)4 );
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
