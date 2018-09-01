#include "world.hpp"

#include "timing.hpp"
#include "shader.hpp"
#include "textureAtlas.hpp"
#include "pointMath.hpp"
#include "hashing.hpp"
#include "meshBuffer.hpp"
#include <unistd.h>

#define MESH_SLEEP_US 10000

World::World(int loadThreads, const Point3i &center, const Vector3i &loadRadius)
  : mLoader(loadThreads, std::bind(&World::chunkLoadCallback, this, std::placeholders::_1)),
    mMeshPool(4, std::bind(&World::meshWorker, this, std::placeholders::_1), MESH_SLEEP_US),
    mCenter(center), mLoadRadius(loadRadius), mChunkDim(loadRadius * 2 + 1),
    mMinChunk(center - loadRadius), mMaxChunk(center + loadRadius),
    mFogStart((loadRadius[0]*Chunk::sizeX)*0.9f),
    mFogEnd((loadRadius[0]*Chunk::sizeX)*1.0f),
    mCenterLoopCallback(std::bind(&World::checkChunkLoad, this, std::placeholders::_1))
{ }

World::~World()
{
  stopLoading();
  for(auto &chunk : mChunks)
    {
      if(chunk.second)
        { delete chunk.second; }
    }
  while(mUnusedChunks.size() > 0)
    {
      Chunk *chunk = mUnusedChunks.pop();
      if(chunk)
        { delete chunk; }
    }
  delete mChunkLineMesh;
}

bool World::loadWorld(std::string worldName, uint32_t seed)
{
  LOGD("SETTING WORLD");
  if(worldName == "")
    {
      std::cout << "Enter world name:  ";
      std::cin >> worldName;
    }
  
  std::vector<std::string> worlds = mLoader.listWorlds();
  bool found = false;
  for(auto w : worlds)
    {
      if(w == worldName)
        {
          found = true;
          break;
        }
    }
  if(!found)
    {
      if(!mLoader.createWorld(worldName, seed))
        {
          LOGD("FAILED TO CREATE WORLD");
          return false;
        }
    }
  if(!mLoader.loadWorld(worldName))
    {
      LOGD("FAILED TO LOAD WORLD");
      return false;
    }
  return true;
}

void World::initGL(QObject *qparent)
{
  
  // load shaders
  mBlockShader = new Shader(qparent);
  if(!mBlockShader->loadProgram("./shaders/simpleBlock.vsh", "./shaders/simpleBlock.fsh",
                                {"posAttr", "normalAttr", "texCoordAttr"},
                                {"pvm", "camPos", "fogStart", "fogEnd", "uTex"} ))
    {
      LOGE("Simple block shader failed to load!");
    }
  mBlockShader->bind();
  mBlockShader->setUniform("uTex", 0);
  mBlockShader->setUniform("fogStart", mFogStart);
  mBlockShader->setUniform("fogEnd", mFogEnd);
  mBlockShader->release();
  
  mChunkLineShader = new Shader(qparent);
  if(!mChunkLineShader->loadProgram("./shaders/chunkLine.vsh", "./shaders/chunkLine.fsh",
                                    {"posAttr", "normalAttr",  "texCoordAttr"},
                                    {"pvm", "camPos", "fogStart", "fogEnd"} ))
    {
      LOGE("Chunk line shader failed to load!");
    }
  mChunkLineShader->bind();
  mChunkLineShader->setUniform("fogStart", mFogStart);
  mChunkLineShader->setUniform("fogEnd", mFogEnd);
  mChunkLineShader->release();

  mChunkLineMesh = new cMeshBuffer();
  mChunkLineMesh->initGL(mChunkLineShader);
  mChunkLineMesh->setMode(GL_LINES);

  // load block textures
  mTexAtlas = new cTextureAtlas(qparent, ATLAS_BLOCK_SIZE);
  if(!mTexAtlas->create("./res/texAtlas.png"))
    { LOGE("Failed to load texture atlas!"); }
}
void World::cleanupGL()
{
  delete mBlockShader;
  delete mChunkLineShader;
  mTexAtlas->destroy();
  delete mTexAtlas;

  std::lock_guard<std::mutex> lock(mRenderLock);
  for(auto mesh : mRenderMeshes)
    {
      mesh.second->cleanupGL();
      delete mesh.second;
    }
  mRenderMeshes.clear();
  while(mUnusedMeshes.size() > 0)
    {
      ChunkMesh *mesh = mUnusedMeshes.pop();
      mesh->cleanupGL();
      delete mesh;
    }
}

#define RENDER_LOAD_MESH_PER_FRAME 8

void World::addMesh(MeshedChunk *mc)
{
  auto iter = mRenderMeshes.find(mc->hash);
  if(iter != mRenderMeshes.end())
    {
      mUnusedMeshes.push(iter->second);
      mRenderMeshes.erase(iter->first);
    }
  
  ChunkMesh *mesh;
  if(mUnusedMeshes.size() > 0)
    { mesh = mUnusedMeshes.pop(); }
  else
    {
      mesh = new ChunkMesh();
      mesh->initGL(mBlockShader);
    }
  mesh->uploadData(mc->mesh);
  mRenderMeshes[mc->hash] = mesh;
  delete mc;
}

void World::render(Matrix4 pvm)
{
  {
    // upload any new chunk meshes
    std::lock_guard<std::mutex> lock(mRenderLock);
    int num = 0;
    for(auto hash : mUnloadMeshes)
      {
        //LOGD("MESH UNLOAD");
        auto iter = mRenderMeshes.find(hash);
        if(iter != mRenderMeshes.end())
          {
            ChunkMesh *mesh = iter->second;
            mRenderMeshes.erase(iter->first);
            mUnusedMeshes.push(mesh);
          }
      }
    while(mRenderQueue.size() > 0)
      {
        MeshedChunk *mc = mRenderQueue.pop();
        addMesh(mc);
        
        if(++num >= RENDER_LOAD_MESH_PER_FRAME)
          { break; }
      }
    mUnloadMeshes.clear();
  }
  
  // render all loaded chunks
  mBlockShader->bind();
  mTexAtlas->bind();
  mBlockShader->setUniform("pvm", pvm);
  mBlockShader->setUniform("camPos", mCamPos);

  bool changed = false;
  if(mRadChanged)
    {
      mBlockShader->setUniform("fogStart", mFogStart);
      mBlockShader->setUniform("fogEnd", mFogEnd);
      changed = true;
      mRadChanged = false;
    }
  {
    //LOGD("RENDERING");
    for(auto &iter : mRenderMeshes)
      {
        /*
        if((!mClipFrustum && mFrustumRender[iter.first]) ||
           (mClipFrustum && mFrustum &&
            mFrustum->cubeInside(Hash::unhash(iter.first)*Chunk::size, Chunk::size, pvm)))
          {
            iter.second->render();
          }
        */
        iter.second->render();
      }
  }
  mBlockShader->release();
  mTexAtlas->release();

  if(mDebug)
    {
      mChunkLineShader->bind();
      mChunkLineMesh->uploadData(makeChunkLineMesh());
      mChunkLineShader->setUniform("pvm", pvm*matTranslate(mMinChunk[0]*Chunk::sizeX,
                                                           mMinChunk[1]*Chunk::sizeY,
                                                           mMinChunk[2]*Chunk::sizeZ ));
      mChunkLineShader->setUniform("camPos", mCamPos);
      if(changed)
        {
          mChunkLineShader->setUniform("fogStart", mFogStart);
          mChunkLineShader->setUniform("fogEnd", mFogEnd);
        }
      
      mChunkLineMesh->render();
      
      mChunkLineShader->release();
    }
}

void World::startLoading()
{
  mMeshPool.start();
  mLoader.start();
}
void World::stopLoading()
{
  mLoader.stop();
  mMeshPool.stop(false);
  mMeshCv.notify_all();
  mMeshPool.stop(true);
}

bool World::checkChunkLoad(const Point3i &cp)
{
  const int cHash = Hash::hash(cp);
  auto iter = mChunks.find(cHash);
  if(iter == mChunks.end())
    {
      //LOGD("LOADING CHUNK");
      //std::cout << "CP: " << cp << ", MIN: " << mMinChunk << ", MAX: " << mMaxChunk << "\n";
      Chunk *chunk = nullptr;
      if(mUnusedChunks.size() > 0)
        {
          chunk = mUnusedChunks.pop();
          chunk->setWorldPos(cp);
        }
      else
        { chunk = new Chunk(cp); }

      //LOGD("LOADING --> %d %d %d", chunk->pos()[0], chunk->pos()[1], chunk->pos()[2]);
      mChunks.emplace(cHash, nullptr);
      mNeighborsLoaded.emplace(cHash, 0);
      mLoader.load(chunk);
      return true;
    }
    
  return false;
}

void World::update()
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  //LOGD("CHUNK UPDATING");
  {
    //std::lock_guard<std::mutex> lock(mRenderLock);
    // see if any chunks within range need to be unloaded
    //LOGD("CHECKING FOR UNLOAD CHUNKS");
    std::vector<int32_t> unload;
    for(auto &iter : mChunks)
      {
        if(iter.second)
          {
            if(!pointInRange(iter.second->pos(), mMinChunk, mMaxChunk))
              {
                unload.push_back(iter.first);
              }
          }
      }
    //LOGD("UNLOAD SIZE: %d", unload.size());
    for(auto cHash : unload)
      {
        auto iter = mChunks.find(cHash);
        if(iter != mChunks.end())
          {
            if(iter->second)
              {
                mUnusedChunks.push(iter->second);
              }
            mChunks.erase(cHash);
            mNeighborsLoaded.erase(cHash);

            {
              std::lock_guard<std::mutex> lock(mMeshLock);
              mMeshing.erase(cHash);
            }
            std::lock_guard<std::mutex> lock(mRenderLock);
            mUnloadMeshes.insert(cHash);
          }
      }
  }
  { // see if any chunks within range need to be loaded
    
    //std::lock_guard<std::mutex> lock(mRenderLock);
    int num = 0;
    num += loopFromCenter(mCenter, mLoadRadius, mCenterLoopCallback);
    //for(int x = mMinChunk[0]; x < mMaxChunk[0]; x++)
    //for(int y = mMinChunk[1]; y < mMaxChunk[1]; y++)
    //  for(int z = mMinChunk[2]; z < mMaxChunk[2]; z++)
    //    { checkChunkLoad(Point3i{x,y,z}); }
  }

  // see if any chunks have finished loading
  {
    int num = 0;
    while(mLoadQueue.size() > 0)
      {
        Chunk *chunk = mLoadQueue.pop();
        if(chunk && pointInRange(chunk->pos(), mMinChunk, mMaxChunk))
          {
            int32_t cHash = Hash::hash(chunk->pos());
            auto iter = mChunks.find(cHash);
            if(iter != mChunks.end() && iter->second)
              {
                mUnusedChunks.push(iter->second);
                mNeighborsLoaded[cHash] = 0;
              }
            //LOGD("  LOADED --> %d %d %d", chunk->pos()[0], chunk->pos()[1], chunk->pos()[2]);
            //LOGD("LOAD CHUNK --> %d %d %d", chunk->pos()[0], chunk->pos()[1], chunk->pos()[2]);
            // update mesh
            if(chunk->isEmpty())
              {
                chunk->setDirty(false);
                chunk->setIncomplete(false);
                chunk->setPriority(false);
              }
            else
              {
                //LOGD("CHUNK NOT EMPTY");
                chunk->setDirty(true);
                chunk->setIncomplete(true);
                chunk->setPriority(false);
              }
            mChunks[cHash] = chunk;
          }
      }
  }
  {
    // update meshes
    int num = 0;
    for(auto &iter : mChunks)
      {
        if(iter.second &&
           ((!iter.second->isIncomplete() && iter.second->isDirty()) ||
            (iter.second->isIncomplete() && (neighborsLoaded(iter.second) == gBlockSides.size()) )))
          {
            if(neighborsLoaded(iter.second) == gBlockSides.size())
              {
                //LOGD("SETTING COMPLETE");
                iter.second->setIncomplete(false);
              }
            {
              std::lock_guard<std::mutex> lock(mMeshLock);

              //LOGD("MESH QUEUING");
              if(mMeshing.count(iter.first) == 0)
                {
                  //LOGD("SETTING COMPLETE");
                  mMeshing.insert(iter.first);
                  iter.second->setDirty(false);
                  if(iter.second->isPriority())
                    { mMeshQueue.push_front(iter); }
                  else
                    { mMeshQueue.push_back(iter); }
                }
            }
            mMeshCv.notify_one();
            //LOGD("DONE UPDATING MESH");
          }
      }
  }
  // TODO: save chunk
}


std::vector<Vector3i> adjacent
  { Vector3i{1,0,0}, Vector3i{0,1,0}, Vector3i{-1,0,0}, Vector3i{0,-1,0}, Vector3i{0,0,-1}, };

void World::step()
{
  if(!mSimFluids)
    { return; }
#define FLOW 0.1
#define ZFLOW 1.0

  std::unordered_map<int32_t, Chunk*> chunks;
  std::unordered_map<int32_t, bool> updates;
  {
    std::lock_guard<std::mutex> lock(mChunkLock);
    chunks = mChunks;
    for(auto &c : chunks)
    { updates.emplace(c.first, false); }
  }
  FluidData *newFluid = new FluidData(0.0f, 0.0f);
  for(auto &iter : chunks)
    {
      Chunk *chunk = iter.second;
      if(chunk)
        {
          bool chunkChanged = false;
          auto fluids = chunk->getFluids();
          for(auto &fIter : fluids)
            {
              FluidData *fData = fIter.second;
              if(fData->gone())
                { continue; }
              int bi = fIter.first;
              Point3i wp = chunk->pos()*Chunk::size + Chunk::indexer().unindex(bi);
              Block *b = chunk->at(bi);
              newFluid->fluidEvap = fData->fluidEvap;

              // first check block underneath
              Point3i np{wp[0], wp[1], wp[2]-1};
              Chunk *nc = getChunk(np);
              if(nc)
                {
                  Block *nb = at(np);
                  bool fluidFull = false;
                  if(nb->type == block_t::NONE)
                    {
                      newFluid->fluidLevel = fData->fluidLevel;
                      setBlock(np, b->type, newFluid);
                      setBlock(wp, block_t::NONE, nullptr);
                      updates[iter.first] = true;
                      updates[Hash::hash(chunkPos(np))] = true;
                    }
                  else if(isFluidBlock(nb->type))
                    { // combine fluids if possible
                      BlockData *nData = getData(np);
                      float overflow = reinterpret_cast<FluidData*>(nData)->adjustLevel(fData->fluidLevel);
                      fData->fluidLevel = overflow;
                      if(overflow > 0.0f)
                        { fluidFull = true; }
                      updates[iter.first] = true;
                      updates[Hash::hash(chunkPos(np))] = true;
                    }

                  if(isSimpleBlock(nb->type) || fluidFull)
                    {
                      Point3i pPX = wp + Vector3i{1,0,0};
                      Chunk *cPX = getChunk(pPX);
                      block_t tPX = getType(pPX);
                      BlockData *bbPX = getData(pPX);
                      FluidData *bPX = (bbPX ? reinterpret_cast<FluidData*>(bbPX) : nullptr);
                      float pressurePX = (bPX ? fData->fluidLevel - bPX->fluidLevel : (tPX == block_t::NONE ?
                                                                                       fData->fluidLevel : 0));

                      Point3i pNX = wp + Vector3i{-1,0,0};
                      Chunk *cNX = getChunk(pNX);
                      block_t tNX = getType(pNX);
                      FluidData *bNX = reinterpret_cast<FluidData*>(getData(pNX));
                      float pressureNX = (bNX ? fData->fluidLevel - bNX->fluidLevel : (tNX == block_t::NONE ?
                                                                                       fData->fluidLevel : 0));

                      Point3i pPY = wp + Vector3i{0,1,0};
                      Chunk *cPY = getChunk(pPY);
                      block_t tPY = getType(pPY);
                      FluidData *bPY = reinterpret_cast<FluidData*>(getData(pPY));
                      float pressurePY = (bPY ? fData->fluidLevel - bPY->fluidLevel : (tPY == block_t::NONE ?
                                                                                       fData->fluidLevel : 0));
              
                      Point3i pNY = wp + Vector3i{0,-1,0};
                      Chunk *cNY = getChunk(pNY);
                      block_t tNY = getType(pNY);
                      FluidData *bNY = reinterpret_cast<FluidData*>(getData(pNY));
                      float pressureNY = (bNY ? fData->fluidLevel - bNY->fluidLevel : (tNY == block_t::NONE ?
                                                                                       fData->fluidLevel : 0));
              
                      float flowFull = 0.25f;
                      float flowEmpty = 0.4f;
                      if(pressurePX > 0)
                        {
                          if(tPX == block_t::NONE)
                            {
                              newFluid->fluidLevel = 0.0f;
                              cPX->setBlock(Chunk::blockPos(pPX), b->type, newFluid);
                              bPX = reinterpret_cast<FluidData*>(getData(pPX));
                            }
                          float flow = lerp(flowEmpty, flowFull, 1.0f - pressurePX);
                          bPX->adjustLevel(pressurePX * flow);
                          fData->adjustLevel(-pressurePX * flow);
                          updates[iter.first] = true;
                          updates[Hash::hash(chunkPos(pPX))] = true;
                        }
                      if(pressureNX > 0)
                        {
                          if(tNX == block_t::NONE)
                            {
                              newFluid->fluidLevel = 0.0f;
                              cNX->setBlock(Chunk::blockPos(pNX), b->type, newFluid);
                              bNX = reinterpret_cast<FluidData*>(getData(pNX));
                            }
                          float flow = lerp(flowEmpty, flowFull, 1.0f - pressureNX);
                          bNX->adjustLevel(pressureNX * flow);
                          fData->adjustLevel(-pressureNX * flow);
                          updates[iter.first] = true;
                          updates[Hash::hash(chunkPos(pNX))] = true;
                        }
                      if(pressurePY > 0)
                        {
                          if(tPY == block_t::NONE)
                            {
                              newFluid->fluidLevel = 0.0f;
                              cPY->setBlock(Chunk::blockPos(pPY), b->type, newFluid);
                              bPY = reinterpret_cast<FluidData*>(getData(pPY));
                            }
                          float flow = lerp(flowEmpty, flowFull, 1.0f - pressurePY);
                          bPY->adjustLevel(pressurePY * flow);
                          fData->adjustLevel(-pressurePY * flow);
                          updates[iter.first] = true;
                          updates[Hash::hash(chunkPos(pPY))] = true;
                        }
                      if(pressureNY > 0)
                        {
                          if(tNY == block_t::NONE)
                            {
                              newFluid->fluidLevel = 0.0f;
                              cNY->setBlock(Chunk::blockPos(pNY), b->type, newFluid);
                              bNY = reinterpret_cast<FluidData*>(getData(pNY));
                            }
                          float flow = lerp(flowEmpty, flowFull, 1.0f - pressureNY);
                          bNY->adjustLevel(pressureNY * flow);
                          fData->adjustLevel(-pressureNY * flow);
                          updates[iter.first] = true;
                          updates[Hash::hash(chunkPos(pNY))] = true;
                        }
                    }
                }
            }

          if(chunk->step(mEvapFluids))
            {
              updates[iter.first] = true;
            }          
        }
    }


  delete newFluid;
  std::lock_guard<std::mutex> lock(mChunkLock);
  for(auto &c : updates)
    {
      if(c.second)
        { mChunks[c.first]->setDirty(true); }
    }
  
}

void World::updateNeighbors(Chunk *chunk)
{
  Point3i cp = chunk->pos();
  int32_t cHash = Hash::hash(cp);
  
  for(auto side : gBlockSides)
    {
      int nHash = Hash::hash(cp + sideDirection(side));
      auto iter = mChunks.find(nHash);
      if(iter != mChunks.end() && iter->second)
        {
          iter->second->setDirty(true);
        }
    }
}
int World::neighborsLoaded(Chunk *chunk)
{
  Point3i cp = chunk->pos();
  int32_t cHash = Hash::hash(cp);
  auto nIter = mNeighborsLoaded.find(cHash);
  if(nIter != mNeighborsLoaded.end())
    {
      int loaded = 0;
      for(auto side : gBlockSides)
        {
          Point3i np = cp + sideDirection(side);
          auto iter = mChunks.find(Hash::hash(np));
          if((iter != mChunks.end() && iter->second))
            {
              loaded++;
            }
        }
      mNeighborsLoaded[cHash] = loaded;
      return loaded;
    }
  else
    { return 0; }
}

void World::updateAdjacent(const Point3i &wp)
{
  const Point3i cp = chunkPos(wp);
  const Point3i cp2 = cp + sideDirection(Chunk::chunkEdge(Chunk::blockPos(wp)));
  const Point3i minP{std::min(cp[0], cp2[0]),
                     std::min(cp[1], cp2[1]),
                     std::min(cp[2], cp2[2]) };
  const Point3i maxP{std::max(cp[0], cp2[0]),
                     std::max(cp[1], cp2[1]),
                     std::max(cp[2], cp2[2]) };
  Point3i p;
  for(p[0] = minP[0]; p[0] <= maxP[0]; p[0]++)
    for(p[1] = minP[1]; p[1] <= maxP[1]; p[1]++)
      for(p[2] = minP[2]; p[2] <= maxP[2]; p[2]++)
        {
          auto iter = mChunks.find(Hash::hash(p));
          if(iter != mChunks.end() && iter->second)
            { iter->second->setDirty(true); }
        }
}

Block* World::at(const Point3i &worldPos)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  Point3i cp = chunkPos(worldPos);
  auto iter = mChunks.find(Hash::hash(cp));
  if(iter != mChunks.end() && iter->second)
    { return iter->second->at(Chunk::blockPos(worldPos)); }
  else
    { return nullptr; }
}
BlockData* World::getData(const Point3i &worldPos)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  Point3i cp = chunkPos(worldPos);
  auto iter = mChunks.find(Hash::hash(cp));
  if(iter != mChunks.end() && iter->second)
    { return iter->second->getData(Chunk::blockPos(worldPos)); }
  else
    { return nullptr; }
}
block_t World::getType(const Point3i &worldPos)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  Point3i cp = chunkPos(worldPos);
  auto iter = mChunks.find(Hash::hash(cp));
  if(iter != mChunks.end() && iter->second)
    { return iter->second->getType(Chunk::blockPos(worldPos)); }
  else
    { return block_t::NONE; }
}

Chunk* World::getChunk(const Point3i &worldPos)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  auto iter = mChunks.find(Hash::hash(chunkPos(worldPos)));
  if(iter != mChunks.end() && iter->second)
    { return iter->second; }
  else
    { return nullptr; }
}

bool World::setBlock(const Point3i &worldPos, block_t type, BlockData *data)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  Point3i cp = chunkPos(worldPos);

  Chunk *chunk = nullptr;
  {
    auto iter = mChunks.find(Hash::hash(cp));
    if(iter != mChunks.end())
      { chunk = iter->second; }
  }
  if(chunk && chunk->setBlock(Chunk::blockPos(worldPos), type, data))
    {
      updateAdjacent(worldPos);
      chunk->setPriority(true);
      return true;
    }
  return false;
}

block_t World::getBlock(const Point3i &wp, std::unordered_map<int32_t, Chunk*> &neighbors)
{
  auto iter = neighbors.find(Hash::hash(chunkPos(wp)));
  if(iter != neighbors.end() && iter->second)
    { return iter->second->getType(Chunk::blockPos(wp)); }
  else
    { return block_t::NONE; }
}
Block* World::atBlock(const Point3i &wp, std::unordered_map<int32_t, Chunk*> &neighbors)
{
  auto iter = neighbors.find(Hash::hash(chunkPos(wp)));
  if(iter != neighbors.end() && iter->second)
    { return iter->second->at(Chunk::blockPos(wp)); }
  else
    { return nullptr; }
}

inline int World::getAO(int e1, int e2, int c)
{
  if(e1 == 1 && e2 == 1)
    { return 3; }
  else
    { return (e1 + e2 + c); }
}


void World::meshWorker(int tid)
{
  std::unique_lock<std::mutex> lock(mMeshLock);
  mMeshCv.wait(lock, [this]{ return !mMeshPool.running() || mMeshQueue.size() > 0; });
  if(!mMeshPool.running())
    { return; }
  
  auto next = mMeshQueue.front();
  mMeshQueue.pop_front();
  mMeshing.erase(next.first);
  Chunk *chunk = next.second;
  mMeshLock.unlock();
  
  if(chunk)
    { updateChunkMesh(chunk, true); }
}

int World::getLighting(const Point3i &bp, const Point3f &vp, blockSide_t side,
                       std::unordered_map<int32_t, Chunk*> &neighbors )
{
  const int dim = sideDim(side); // dimension of face normal
  const int dim1 = (dim+1) % 3;  // dimension of edge 1
  const int dim2 = (dim+2) % 3;  // dimension of edge 2

  const int e1 = bp[dim1] + (int)vp[dim1] * 2 - 1; // 
  const int e2 = bp[dim2] + (int)vp[dim2] * 2 - 1;
  
  Point3i bi;
  bi[dim] = bp[dim] + sideSign(side);
  bi[dim1] = e1;
  bi[dim2] = e2;

  int lc;
  int le1;
  int le2;

  lc = (isSimpleBlock(getBlock(bi, neighbors)) ? 1 : 0);
  
  bi[dim1] = bp[dim1];
  le1 = (isSimpleBlock(getBlock(bi, neighbors)) ? 1 : 0);
  
  bi[dim1] = e1;
  bi[dim2] = bp[dim2];
  le2 = (isSimpleBlock(getBlock(bi, neighbors)) ? 1 : 0);
  
  return 3 - getAO(le1, le2, lc);
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

void addChunkFace(MeshData &data, const Point3i &cp)
{
  int nVert = data.vertices.size();
  data.vertices.emplace_back((cp + Point3i{0,0,0})*Chunk::size, Point3i{0,0,0}, Point2i{0,0});
  data.vertices.emplace_back((cp + Point3i{0,1,0})*Chunk::size, Point3i{0,0,0}, Point2i{0,0});
  data.vertices.emplace_back((cp + Point3i{1,1,0})*Chunk::size, Point3i{0,0,0}, Point2i{0,0});
  data.vertices.emplace_back((cp + Point3i{1,0,0})*Chunk::size, Point3i{0,0,0}, Point2i{0,0});
  
  data.vertices.emplace_back((cp + Point3i{0,0,1})*Chunk::size, Point3i{0,0,0}, Point2i{0,0});
  data.vertices.emplace_back((cp + Point3i{0,1,1})*Chunk::size, Point3i{0,0,0}, Point2i{0,0});
  data.vertices.emplace_back((cp + Point3i{1,1,1})*Chunk::size, Point3i{0,0,0}, Point2i{0,0});
  data.vertices.emplace_back((cp + Point3i{1,0,1})*Chunk::size, Point3i{0,0,0}, Point2i{0,0});

  data.indices.push_back(nVert + 0);
  data.indices.push_back(nVert + 1);
  data.indices.push_back(nVert + 1);
  data.indices.push_back(nVert + 2);
  data.indices.push_back(nVert + 2);
  data.indices.push_back(nVert + 3);
  data.indices.push_back(nVert + 3);
  data.indices.push_back(nVert + 0);

  data.indices.push_back(nVert + 4);
  data.indices.push_back(nVert + 5);
  data.indices.push_back(nVert + 5);
  data.indices.push_back(nVert + 6);
  data.indices.push_back(nVert + 6);
  data.indices.push_back(nVert + 7);
  data.indices.push_back(nVert + 7);
  data.indices.push_back(nVert + 4);

  data.indices.push_back(nVert + 0);
  data.indices.push_back(nVert + 4);
  data.indices.push_back(nVert + 1);
  data.indices.push_back(nVert + 5);
  data.indices.push_back(nVert + 2);
  data.indices.push_back(nVert + 6);
  data.indices.push_back(nVert + 3);
  data.indices.push_back(nVert + 7);
  
}

MeshData World::makeChunkLineMesh()
{
  MeshData data;
  std::unordered_set<int32_t> mDone;

  std::lock_guard<std::mutex> lock(mRenderLock);
  //LOGD("CHUNK LINE MESH");
  int maxR = std::max(mLoadRadius[0], std::max(mLoadRadius[1], mLoadRadius[2]));
  const Point3i offset = mMaxChunk-1;

  Point3i cp;
  for(cp[0] = 0; cp[0] < mChunkDim[0]; cp[0]++)
    for(cp[1] = 0; cp[1] < mChunkDim[1]; cp[1]++)
      for(cp[2] = 0; cp[2] < mChunkDim[2]; cp[2]++)
        {
          auto iter = mRenderMeshes.find(Hash::hash(mMinChunk+cp));
          if(iter != mRenderMeshes.end() && iter->second)
            { addChunkFace(data, cp); }
        }
  //LOGD("DONE CHUNK LINE MESH");
  return data;
}
void World::updateChunkMesh(Chunk *chunk, bool priority)
{
  const Point3i cPos = chunk->pos();
  const int32_t cHash = Hash::hash(cPos);

  MeshedChunk *mc = new MeshedChunk({cHash, MeshData()});

  static const std::array<blockSide_t, 6> sides {{ blockSide_t::PX,
                                                   blockSide_t::PY,
                                                   blockSide_t::PZ,
                                                   blockSide_t::NX,
                                                   blockSide_t::NY,
                                                   blockSide_t::NZ }};
  static const std::array<Point3i, 6> sideDirections {{ Point3i{1,0,0},
                                                        Point3i{0,1,0},
                                                        Point3i{0,0,1},
                                                        Point3i{-1,0,0},
                                                        Point3i{0,-1,0},
                                                        Point3i{0,0,-1} }};
  // get neighbors
  std::unordered_map<int32_t, Chunk*> neighbors;
  {
    std::lock_guard<std::mutex> lock(mChunkLock);
    Point3i np;
    for(np[0] = cPos[0]-1; np[0] <= cPos[0]+1; np[0]++)
      for(np[1] = cPos[1]-1; np[1] <= cPos[1]+1; np[1]++)
        for(np[2] = cPos[2]-1; np[2] <= cPos[2]+1; np[2]++)
          {
            int cHash = Hash::hash(np);
            auto iter = mChunks.find(cHash);
            if(iter != mChunks.end() && iter->second)
              { neighbors.emplace(cHash, iter->second); }
          }
  }
  
  // iterate over the chunk's blocks and compile all face vertices.
  Point3i bp;
  //const Point3i minP = cPos `+ Chunk::blockPos(cPos*Chunk::size);
  const Point3i minP{cPos[0]*Chunk::sizeX,
                     cPos[1]*Chunk::sizeY,
                     cPos[2]*Chunk::sizeZ};
  const Point3i maxP = minP + Chunk::size;
  for(bp[0] = 0; bp[0] < Chunk::size[0]; bp[0]++)
    for(bp[1] = 0; bp[1] < Chunk::size[1]; bp[1]++)
      for(bp[2] = 0; bp[2] < Chunk::size[2]; bp[2]++)
	{
	  CompleteBlock block = {chunk->getType(bp), chunk->getData(bp)};
          
          if(block.type != block_t::NONE)
            { // block is not empty
              //LOGD("BLOCK (%d, %d, %d) ACTIVE", bp[0], bp[1], bp[2]);
              
              for(int i = 0; i < 6; i++)
                {
                  const Point3i np = bp + sideDirections[i];
                  /*
                  auto iter = mChunks.find(Hash::hash(chunkPos(np)));
                  if(iter != mChunks.end() && iter->second &&
                     iter->second->get(Chunk::blockPos(np)) == block_t::NONE )
                  */
                  block_t nt = getBlock(minP + np, neighbors);
                  if(/*block.type != nt && */(!isSimpleBlock(block.type) || !isSimpleBlock(nt)))
                    { // block on side i is empty (face is active)
                      const Point3i vOffset = minP + bp;
                      const unsigned int numVert = mc->mesh.vertices.size();

                      Vector3f offset;
                      if(block.data)
                        {
                          if(isFluidBlock(block.type))
                            {
#define MIN_MESH_LEVEL 0.005
                              float level = reinterpret_cast<FluidData*>(block.data)->fluidLevel;
                              if(level < MIN_MESH_LEVEL ||
                                 reinterpret_cast<FluidData*>(block.data)->gone() )
                                { continue; }
                              offset[2] = (float)level - 1.0f;
                            }
                          //std::cout << offset << "\n";
                        }

                      int vn = 0;
                      int sum0 = 0;
                      int sum1 = 0;
                      for(auto v : faceVertices[sides[i]] )
                        { // add vertices for this face

                          //std::cout << v.pos << "\n";

                          Point3f p = minP + bp + v.pos; 
                          p[2] += (v.pos[2] > 0 ? offset[2] : 0.0f);
                          const int lighting = getLighting(minP + bp, v.pos, sides[i], neighbors);
                          
                          mc->mesh.vertices.emplace_back(p,
                                                         v.normal,
                                                         v.texcoord+offset,
                                                         (int)block.type - 1,
                                                         (float)lighting / (float)4 );

                          //std::cout << mc->mesh.vertices.back().pos << "\n";
                          
                          if(vn < 2) { sum0 += lighting; }
                          else       { sum1 += lighting; }
                          vn++;
                        }
                      if(sum1 >= sum0)
                        {
                          for(auto i : flippedIndices)
                            { mc->mesh.indices.push_back(numVert + i); }
                        }
                      else
                        {
                          for(auto i : faceIndices)
                            { mc->mesh.indices.push_back(numVert + i); }
                        }
                    }
                  // TODO: Complex blocks / etc...
                }
            }
        }
  //LOGD("MESH VERTICES: %d, INDICES: %d", mc->mesh.vertices.size(), mc->mesh.indices.size());

  // pass to render thread
  if(!mc->mesh.empty())
    {
      //LOGD("VALID MESH!");
      mRenderQueue.push(mc);
    }
  else
    {
      //LOGD("SKIPPING EMPTY MESH!");
      delete mc;
    }
  //LOGD("DONE PUSHING MESH!");
}

void World::chunkLoadCallback(Chunk *chunk)
{
  //LOGD("CHUNK LOAD CALLBACK --> %d", (long)chunk);
  //  mChunks[Hash::hash(chunk->pos())] = chunk;
  mLoadQueue.push(chunk);
}

bool World::readyForPlayer() const
{
  return true; //(mChunks.numLoaded() > 0);
}

Point3f World::getStartPos(const Point3i &pPos)
{
  return Point3f{pPos[0], pPos[1], pPos[2]};//getHeightAt(pPos)};
}

void World::setCenter(const Point3i &chunkCenter)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  LOGI("Moving center from (%d, %d, %d) to (%d, %d, %d)", mCenter[0], mCenter[1], mCenter[2],
       chunkCenter[0], chunkCenter[1], chunkCenter[2] );
  mCenter = chunkCenter;
  mMinChunk = mCenter - mLoadRadius;
  mMaxChunk = mCenter + mLoadRadius;
}
void World::setRadius(const Vector3i &chunkRadius)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  LOGI("Setting chunk radius to (%d, %d, %d)", chunkRadius[0], chunkRadius[1], chunkRadius[2]);
  mLoadRadius = chunkRadius;
  mChunkDim = mLoadRadius * 2 + 1;
  mMinChunk = mCenter - mLoadRadius;
  mMaxChunk = mCenter + mLoadRadius;

  mFogStart = (mLoadRadius[0]*Chunk::sizeX)*0.9f;
  mFogEnd = (mLoadRadius[0]*Chunk::sizeX)*1.0f;
  mRadChanged = true;
}
Point3i World::getCenter() const
{ return mCenter; }

void World::setFrustum(Frustum *frustum)
{ mFrustum = frustum; }
void World::setFrustumClip(bool on)
{ mClipFrustum = on; }

void World::setFrustumPause()
{
  std::lock_guard<std::mutex> lock(mRenderLock);
  Matrix4 pvm = mFrustum->getProjection() * mFrustum->getView();
  if(mClipFrustum)
    {
      mFrustumRender.clear();
      int num = 0;
      for(auto &iter : mRenderMeshes)
        {
          if(mFrustum->cubeInside(Hash::unhash(iter.first)*Chunk::size, Chunk::size, pvm))
            { mFrustumRender[iter.first] = true; num++; }
          else
            { mFrustumRender[iter.first] = false; }
        }
      mClipFrustum = false;
      std::cout << "FRUSTUM PAUSED --> " << num << " / " << mRenderMeshes.size() << " chunks rendered.\n";
    }
  else
    {
      mClipFrustum = true;
      std::cout << "FRUSTUM UNPAUSED\n";
    }
}


int World::chunkX(int wx)
{ return wx >> Chunk::shiftX; }
int World::chunkY(int wy)
{ return wy >> Chunk::shiftY; }
int World::chunkZ(int wz)
{ return wz >> Chunk::shiftZ; }
Point3i World::chunkPos(const Point3i &wp)
{ return Point3i({chunkX(wp[0]), chunkY(wp[1]), chunkZ(wp[2])}); }


// Ray casting utils
static float mod(float value, float modulus)
{ return std::fmod(value, modulus); }
static float intbound(float s, float ds)
{
  return (ds > 0 ?
	  (std::floor(s) + 1 - s) / std::abs(ds) :
	  (ds <= 0 ? (s - std::floor(s)) / std::abs(ds) : 0));
}
bool World::rayCast(const Point3f &p, const Vector3f &d, float radius,
                    CompleteBlock &blockOut, Point3i &posOut, Vector3i &faceOut )
{
  //LOGD("RAYCASTING...");
  if(d[0] == 0 && d[1] == 0 && d[2] == 0)
    { return false; }
  
  Vector3i step{  (d[0] > 0 ? 1 : (d[0] < 0 ? -1 : 1)),
		  (d[1] > 0 ? 1 : (d[1] < 0 ? -1 : 1)),
		  (d[2] > 0 ? 1 : (d[2] < 0 ? -1 : 1)) };
  Vector3f tMax{intbound(p[0], d[0]), intbound(p[1], d[1]), intbound(p[2], d[2])};
  Vector3f delta{d[0] == 0 ? 0 : (step[0] / d[0]),
		 d[1] == 0 ? 0 : (step[1] / d[1]),
		 d[2] == 0 ? 0 : (step[2] / d[2]) };
  
  radius /= d.length();
  
  Point3f pi{std::floor(p[0]), std::floor(p[1]), std::floor(p[2])};
  Point3i chunkMin = mMinChunk;
  Point3i chunkMax = mMaxChunk;
  
  while((step[0] > 0 ? (pi[0] <= chunkMax[0]*Chunk::sizeX) : (pi[0] >= chunkMin[0]*Chunk::sizeX)) &&
	(step[1] > 0 ? (pi[1] <= chunkMax[1]*Chunk::sizeY) : (pi[1] >= chunkMin[1]*Chunk::sizeY)) &&
	(step[2] > 0 ? (pi[2] <= chunkMax[2]*Chunk::sizeZ) : (pi[2] >= chunkMin[2]*Chunk::sizeZ)) )
    {
      if(!(pi[0] < chunkMin[0]*Chunk::sizeX ||
	   pi[1] < chunkMin[1]*Chunk::sizeY ||
	   pi[2] < chunkMin[2]*Chunk::sizeZ ||
	   pi[0] > chunkMax[0]*Chunk::sizeX ||
	   pi[1] > chunkMax[1]*Chunk::sizeY ||
	   pi[2] > chunkMax[2]*Chunk::sizeZ ))
	{
          //LOGD("GETTING BLOCK AT");
	  Block *b = at(pi);
          //LOGD("BLOCK: %d", (long)b);
	  if(b && b->type != block_t::NONE)// &&
            //!(isFluidBlock(b->type) && reinterpret_cast<FluidData*>(getData(pi))->fluidLevel < 0.005))
	    {
	      blockOut.type = b->type;
              blockOut.data = getData(pi);
	      posOut = pi;
	      return true;
	    }
	}
	
      if(tMax[0] < tMax[1])
	{
	  if(tMax[0] < tMax[2])
	    {
	      if(tMax[0] > radius)
		{ break; }
	      pi[0] += step[0];
	      tMax[0] += delta[0];
	      faceOut = Vector3i{-step[0], 0, 0};
	    }
	  else
	    {
	      if(tMax[2] > radius)
		{ break; }
	      pi[2] += step[2];
	      tMax[2] += delta[2];
	      faceOut = Vector3i{0, 0, -step[2]};
	    }
	}
      else
	{
	  if(tMax[1] < tMax[2])
	    {
	      if(tMax[1] > radius)
		{ break; }
	      pi[1] += step[1];
	      tMax[1] += delta[1];
	      faceOut = Vector3i{0, -step[1], 0};
	    }
	  else
	    {
	      if(tMax[2] > radius)
		{ break; }
	      pi[2] += step[2];
	      tMax[2] += delta[2];
	      faceOut = Vector3i{0, 0, -step[2]};
	    }
	}
    }
  return false;
}
