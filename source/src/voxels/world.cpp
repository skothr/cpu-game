#include "world.hpp"

#include "timing.hpp"
#include "shader.hpp"
#include "textureAtlas.hpp"
#include "chunkLoader.hpp"
#include "pointMath.hpp"
#include "hashing.hpp"
#include "meshBuffer.hpp"
#include "params.hpp"
#include "fluid.hpp"
#include "fluidChunk.hpp"
#include <unistd.h>

World::World()
  : mLoader(new ChunkLoader(1, std::bind(&World::chunkLoadCallback, this, std::placeholders::_1))),
    mCenterLoopCallback(std::bind(&World::checkChunkLoad, this, std::placeholders::_1)),
    mMeshPool(1, std::bind(&World::meshWorker, this, std::placeholders::_1),
              MESH_THREAD_SLEEP_MS*1000)
{
  mFluids.setRange(mMinChunk, mMaxChunk);
}

World::~World()
{
  stop();
  mLoader->flush();
  delete mLoader;
  for(auto &chunk : mChunks)
    {
      if(chunk.second)
        { delete chunk.second; }
    }
  while(mLoadQueue.size() > 0)
    {
      Chunk *chunk = mLoadQueue.front();
      mLoadQueue.pop();
      if(chunk)
        { delete chunk; }
    }
  while(mUnusedChunks.size() > 0)
    {
      Chunk *chunk = mUnusedChunks.front();
      mUnusedChunks.pop();
      if(chunk)
        { delete chunk; }
    }
}

void World::start()
{
  mLoader->start();
  mMeshPool.start();
}
void World::stop()
{
  mLoader->stop();
  mMeshPool.stop(false);
  mMeshCv.notify_all();
  mMeshPool.stop(true);
}

std::vector<World::Options> World::getWorlds() const
{
  return mLoader->getWorlds();
}
bool World::worldExists(const std::string &name) const
{
  std::vector<std::string> worlds = mLoader->listWorlds();
  for(auto &w : worlds)
    {
      if(w == name)
        { return true; }
    }
  return false;
}
void World::updateInfo(const World::Options &opt)
{
  mLoader->setThreads(opt.loadThreads);
  mMeshPool.setThreads(opt.meshThreads);
  mLoadRadius = opt.chunkRadius;
  mChunkDim = mLoadRadius * 2 + 1;
  mMinChunk = mCenter - mLoadRadius;
  mMaxChunk = mCenter + mLoadRadius;
  mFluids.setRange(mMinChunk, mMaxChunk);

  Vector3f bSize = mLoadRadius*Chunk::size;
  if(bSize[0] == 0) bSize[0] += Chunk::sizeX/2;
  if(bSize[1] == 0) bSize[1] += Chunk::sizeY/2;
  if(bSize[2] == 0) bSize[2] += Chunk::sizeZ/2;
  float minDim = std::min(bSize[0], std::min(bSize[1], bSize[2]));
  mDirScale = Vector3f{minDim/bSize[0],minDim/bSize[1],minDim/bSize[2]};
  mDirScale *= mDirScale;
  mFogStart = (minDim)*0.98f;
  mFogEnd = (minDim)*1.0f;

  // mFogStart = (mLoadRadius[0]*Chunk::sizeX)*0.9f;
  // mFogEnd = (mLoadRadius[0]*Chunk::sizeX)*1.0f;
}
bool World::loadWorld(const World::Options &opt)
{
  LOGI("Loading world...");
  if(opt.name == "")
    {
      LOGW("Please provide a world name!");
      return false;
    }
  else if(!worldExists(opt.name))
    {
      LOGW("Failed to load -- world doesn't exist!");
      return false;
    }
  else if(!mLoader->loadWorld(opt.name))
    {
      LOGE("Failed to load world!");
      return false;
    }
  else
    {
      updateInfo(opt);
      LOGI("Done loading world.");
      return true;
    }
}

bool World::createWorld(const World::Options &opt, bool overwrite)
{
  if(opt.name == "")
    {
      LOGW("Please provide a world name!");
      return false;
    }
  else if(!loadWorld(opt) || overwrite)
    {
      LOGI("Creating world...");
      if(!mLoader->createWorld(opt.name, opt.terrain, opt.seed))
        {
          LOGE("Failed to create world!");
          return false;
        }
      else if(!mLoader->loadWorld(opt.name))
        {
          LOGE("Failed to create world!");
          return false;
        }
          
      updateInfo(opt);
      LOGI("Done creating world.");
      return true;
    }
  else
    {
      if(overwrite)
        { LOGW("World exists -- no overwrite"); }
      return false;
    }
}



void World::initGL(QObject *qparent)
{
  if(!mInitialized)
    {
      // load shaders
      mBlockShader = new Shader(qparent);
      if(!mBlockShader->loadProgram("./shaders/simpleBlock.vsh", "./shaders/simpleBlock.fsh",
                                    {"posAttr", "normalAttr", "texCoordAttr"},
                                    {"pvm", "camPos", "fogStart", "fogEnd", "uTex", "dirScale"} ))
        {
          LOGE("Simple block shader failed to load!");
        }
      mBlockShader->bind();
      mBlockShader->setUniform("uTex", 0);
      mBlockShader->setUniform("fogStart", mFogStart);
      mBlockShader->setUniform("fogEnd", mFogEnd);
      mBlockShader->setUniform("dirScale", mDirScale);
      mBlockShader->release();
  
      mChunkLineShader = new Shader(qparent);
      if(!mChunkLineShader->loadProgram("./shaders/chunkLine.vsh", "./shaders/chunkLine.fsh",
                                        {"posAttr", "normalAttr",  "texCoordAttr"},
                                        {"pvm", "camPos", "fogStart", "fogEnd", "dirScale"} ))
        {
          LOGE("Chunk line shader failed to load!");
        }
      mChunkLineShader->bind();
      mChunkLineShader->setUniform("fogStart", mFogStart);
      mChunkLineShader->setUniform("fogEnd", mFogEnd);
      mChunkLineShader->setUniform("dirScale", mDirScale);
      mChunkLineShader->release();

      mChunkLineMesh = new cMeshBuffer();
      mChunkLineMesh->initGL(mChunkLineShader);
      mChunkLineMesh->setMode(GL_LINES);

      // load block textures
      mTexAtlas = new cTextureAtlas(qparent, ATLAS_BLOCK_SIZE);
      if(!mTexAtlas->create("./res/texAtlas.png"))
        { LOGE("Failed to load texture atlas!"); }
      
      mInitialized = true;
    }
}
void World::cleanupGL()
{
  if(mInitialized)
    {
      delete mChunkLineMesh;
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
          ChunkMesh *mesh = mUnusedMeshes.front();
          mUnusedMeshes.pop();
          mesh->cleanupGL();
          delete mesh;
        }
      std::lock_guard<std::mutex> llock(mUnusedMCLock);
      while(mUnusedMC.size() > 0)
        {
          MeshedChunk *mc = mUnusedMC.front();
          mUnusedMC.pop();
          delete mc;
        }
      mInitialized = false;
    }
}

#define RENDER_LOAD_MESH_PER_FRAME 2

void World::addMesh(MeshedChunk *mc)
{
  auto iter = mRenderMeshes.find(mc->hash);
  if(iter != mRenderMeshes.end())
    {
      mUnusedMeshes.push(iter->second);
      mRenderMeshes.erase(iter->first);
    }
  else
    {
      std::lock_guard<std::mutex> lock(mMeshedLock);
      mMeshed.insert(mc->hash);
    }
  
  ChunkMesh *mesh;
  if(mUnusedMeshes.size() > 0)
    {
      mesh = mUnusedMeshes.front();
      mUnusedMeshes.pop();
    }
  else
    {
      mesh = new ChunkMesh();
      mesh->initGL(mBlockShader);
    }
  mesh->uploadData(mc->mesh);
  mRenderMeshes[mc->hash] = mesh;

  std::lock_guard<std::mutex> lock(mUnusedMCLock);
  mUnusedMC.push(mc);
}

void World::render(Matrix4 pvm)
{
  {
    int num = 0;
    std::lock_guard<std::mutex> lock(mRenderQueueLock);
    while(mRenderQueue.size() > 0)
      {
        MeshedChunk *mc = mRenderQueue.front();
        mRenderQueue.pop();
        addMesh(mc);
        
        if(++num >= RENDER_LOAD_MESH_PER_FRAME)
          { break; }
      }
    // update fluid meshes
    auto updates = mFluids.getUpdates();
    for(auto iter : updates)
      {
        int32_t hash = iter.first;
        MeshData *data = iter.second;
        auto iter2 = mFluidMeshes.find(hash);
        if(iter2 != mFluidMeshes.end())
          {
            mUnusedMeshes.push(iter2->second);
            mFluidMeshes.erase(iter2->first);
          }

        if(data)
          { // null data means to remove mesh.
            ChunkMesh *mesh;
            if(mUnusedMeshes.size() > 0)
              {
                mesh = mUnusedMeshes.front();
                mUnusedMeshes.pop();
              }
            else
              {
                mesh = new ChunkMesh();
                mesh->initGL(mBlockShader);
              }
            mesh->uploadData(*data);
            delete data;
            mFluidMeshes.emplace(hash, mesh);
          }
      }
  }
  {
    std::lock_guard<std::mutex> lock(mUnloadMeshLock);
    for(auto hash : mUnloadMeshes)
      {
        auto iter = mRenderMeshes.find(hash);
        if(iter != mRenderMeshes.end())
          {
            ChunkMesh *mesh = iter->second;
            mRenderMeshes.erase(hash);
            mUnusedMeshes.push(mesh);
            
            std::lock_guard<std::mutex> lock(mMeshedLock);
            mMeshed.erase(iter->first);
          }
        auto fIter = mFluidMeshes.find(hash);
        if(fIter != mFluidMeshes.end())
          {
            ChunkMesh *mesh = fIter->second;
            mFluidMeshes.erase(hash);
            mUnusedMeshes.push(mesh);
          }
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
      mBlockShader->setUniform("dirScale", mDirScale);
      mChunkLineShader->setUniform("fogStart", mFogStart);
      mChunkLineShader->setUniform("fogEnd", mFogEnd);
      mChunkLineShader->setUniform("dirScale", mDirScale);
      changed = true;
      mRadChanged = false;
    }
  {
    std::lock_guard<std::mutex> lock(mRenderLock);
    for(auto &iter : mRenderMeshes)
      {
        iter.second->render();
      }
    //LOGDC(COLOR_GREEN, "FLUID MESHES: %d", mFluidMeshes.size());
    for(auto &iter : mFluidMeshes)
      {
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


int World::numMeshed()
{
  int num = 0;
  std::lock_guard<std::mutex> lock(mMeshedLock);
  for(auto &m : mMeshed)
    { num++; }
  return num;
}


bool World::checkChunkLoad(const Point3i &cp)
{
  const int cHash = Hash::hash(cp);
  auto iter = mChunks.find(cHash);
  if(iter == mChunks.end())
    {
      Chunk *chunk = nullptr;
      if(mUnusedChunks.size() > 0)
        {
          chunk = mUnusedChunks.front();
          mUnusedChunks.pop();
          chunk->setWorldPos(cp);
        }
      else
        { chunk = new Chunk(cp); }

      mChunks.emplace(cHash, nullptr);
      std::unordered_map<blockSide_t, Chunk*> neighbors;
      for(auto &side : gBlockSides)
        { neighbors.emplace(side, nullptr); }
      mNeighbors.emplace(cHash, neighbors);
      mLoader->load(chunk);
      return true;
    }
    
  return false;
}

void World::update()
{
  std::vector<std::pair<int32_t, Chunk*>> unload;
  {
    std::lock_guard<std::mutex> lock(mChunkLock);
    // find any chunks that are out of range to unload
    for(auto &iter : mChunks)
      {
        if(!pointInRange(Hash::unhash(iter.first), mMinChunk, mMaxChunk))
          { unload.push_back(iter); }
      }
  }
  {
    // unload the chunks
    for(auto uIter : unload)
      {
        int32_t cHash = uIter.first;
        Chunk *chunk = uIter.second;
        if(chunk)
          {
            if(chunk->needsSave())
              {
                chunk->setNeedSave(false);
                mLoader->save(chunk);
              }
            {
              std::lock_guard<std::mutex> lock(mUnloadMeshLock);
              mUnloadMeshes.insert(cHash);
            }
            for(auto sIter : mNeighbors[cHash])
              { // remove this chunk from neighbors
                blockSide_t side = sIter.first;
                if(sIter.second)
                  {
                    sIter.second->setIncomplete(true);
                    const int32_t nHash = Hash::hash(sIter.second->pos());
                    mNeighbors[nHash][oppositeSide(side)] = nullptr;
                  }
              }
            mNumLoaded--;
            mUnusedChunks.push(chunk);
          }
        std::lock_guard<std::mutex> lock(mChunkLock);
        mChunks.erase(cHash);
        mNeighbors.erase(cHash);
      }
  }
  
  { // start loading chunks 
    //std::lock_guard<std::mutex> lock(mChunkLock);
    const std::vector<Point3i> &distPoints = getCenterDistPoints(mCenter, mLoadRadius);
    if(mCenterDistIndex < distPoints.size())
      {
        for(int i = mCenterDistIndex; i < distPoints.size(); i++, mCenterDistIndex++)
          {
            Point3i p = distPoints[i] + mCenter;
            const int cHash = Hash::hash(p);
            auto iter = mChunks.find(cHash);
            if(iter == mChunks.end())
              {
                Chunk *chunk = nullptr;
                if(mUnusedChunks.size() > 0)
                  {
                    chunk = mUnusedChunks.front();
                    mUnusedChunks.pop();
                    chunk->setWorldPos(p);
                  }
                else
                  { chunk = new Chunk(p); }

                mChunks.emplace(cHash, nullptr);
                std::unordered_map<blockSide_t, Chunk*> neighbors;
                for(auto &side : gBlockSides)
                  { neighbors.emplace(side, nullptr); }
                mNeighbors.emplace(cHash, neighbors);
                
                mLoader->load(chunk);
                if(++mNumLoading >= mLoader->numThreads())
                { break; }
              }
          }
      }
  }
  
  while(true) // activate chunks that are done loading
    {
      Chunk *chunk = nullptr;
      {
        std::lock_guard<std::mutex> lock(mLoadLock);
        if(mLoadQueue.size() > 0)
          {
            chunk = mLoadQueue.front();
            mLoadQueue.pop();
          }
        else
          { break; }
      }
      if(chunk && pointInRange(chunk->pos(), mMinChunk, mMaxChunk))
        {
          const int32_t cHash = Hash::hash(chunk->pos());
          {
            std::lock_guard<std::mutex> lock(mChunkLock);
            auto iter = mChunks.find(cHash);
            if(iter != mChunks.end())
              {
                if(iter->second)
                  {
                    mUnusedChunks.push(iter->second);
                  }
                else
                  { mNumLoaded++; }
                  
                iter->second = chunk;
                  
                // add neighbors
                for(auto &side : gBlockSides)
                  {
                    const int32_t nHash = Hash::hash(chunk->pos() + sideDirection(side));
                    auto ncIter = mChunks.find(nHash);
                    if(ncIter != mChunks.end())
                      {
                        mNeighbors[cHash][side] = ncIter->second;
                        mNeighbors[nHash][oppositeSide(side)] = chunk;
                      }
                  }
                  
                if(chunk->isEmpty())
                  { // empty mesh
                    chunk->setDirty(true);
                    chunk->setIncomplete(false);
                  }
                else
                  {
                    chunk->setDirty(true);
                    chunk->setIncomplete(true);
                  }
              }
            else
              { LOGW("LOADED CHUNK DESTROYED"); }
          }
        }
    }
  
  { // update dirty meshes
    std::lock_guard<std::mutex> lock(mChunkLock);
    for(auto &iter : mChunks)
      {
        Chunk *chunk = iter.second;
        const int32_t cHash = iter.first;
        if(chunk && !chunk->isEmpty())
          {
            Point3i cp = chunk->pos();
            bool nLoaded = neighborsLoaded(cHash, chunk);
            if(chunk->isIncomplete() && nLoaded)
              {
                chunk->setIncomplete(false);
                chunk->setDirty(true);
              }
            else if(!chunk->isIncomplete() && !nLoaded)
              {
                chunk->setIncomplete(true);
                std::lock_guard<std::mutex> lock(mUnloadMeshLock);
                mUnloadMeshes.insert(iter.first);
              }
            if(!chunk->isIncomplete())
              { // neighbors loaded
                if(chunk->isDirty())
                  { // mesh needs updating
                    std::unique_lock<std::mutex> lock(mMeshLock);
                    chunk->setDirty(false);
                    if(mMeshing.count(cHash) == 0)
                      {
                        meshChunk(chunk);
                        mMeshing.insert(cHash);
                      }
                    lock.unlock();
                    mMeshCv.notify_one();
                  }
              }
          }
      }
  }


  
  { // save any chunks that have been modified
    std::lock_guard<std::mutex> lock(mChunkLock);
    for(auto &iter : mChunks)
      {
        if(iter.second && iter.second->needsSave())
          {
            //LOGD("SAVING");
            mLoader->save(iter.second);
            iter.second->setNeedSave(false);
          }
      }
  }
}

void World::meshChunk(Chunk *chunk)
{
  // keep sorted from farthest to nearest.
  const Point3i cPos = chunk->pos();
  const int dist = (cPos - mCenter).length();
  for(int i = mMeshQueue.size()-1; i >= 0; i--)
    {
      if(dist > (mMeshQueue[i]->pos() - mCenter).length())
        {
          mMeshQueue.insert(mMeshQueue.begin() + i, chunk);
          return;
        }
    }
  mMeshQueue.insert(mMeshQueue.begin() + 0, chunk);
}

void World::step()
{
  if(!mSimFluids)
    { return; }

  std::unordered_map<int32_t, bool> updates = mFluids.step(mEvapFluids);
  
  std::lock_guard<std::mutex> lock(mChunkLock);
  for(auto &c : updates)
    {
      if(c.second)
        {
          auto iter = mChunks.find(c.first);
          if(iter != mChunks.end() && iter->second)
            { iter->second->setDirty(true); }
        }
    }
}

blockSide_t World::getEdges(int32_t hash)
{
  Point3i cp = Hash::unhash(hash);
  blockSide_t edges = ((cp[0] == mMinChunk[0] ? blockSide_t::NX : blockSide_t::NONE) |
                       (cp[1] == mMinChunk[1] ? blockSide_t::NY : blockSide_t::NONE) |
                       (cp[2] == mMinChunk[2] ? blockSide_t::NZ : blockSide_t::NONE) |
                       (cp[0] == mMaxChunk[0] ? blockSide_t::PX : blockSide_t::NONE) |
                       (cp[1] == mMaxChunk[1] ? blockSide_t::PY : blockSide_t::NONE) |
                       (cp[2] == mMaxChunk[2] ? blockSide_t::PZ : blockSide_t::NONE) );
  return edges;
}

bool World::neighborsLoaded(int32_t hash, Chunk *chunk)
{
  blockSide_t edgeSides = getEdges(hash);
  auto nIter = mNeighbors.find(hash);
  if(nIter != mNeighbors.end())
    {
      int loaded = 0;
      for(auto side : gBlockSides)
        {
          if(((edgeSides & side) != blockSide_t::NONE) || nIter->second[side])
            { loaded++; }
          else
            {
              auto cIter = mChunks.find(Hash::hash(chunk->pos() + sideDirection(side)));
              if(cIter != mChunks.end())
                { nIter->second[side] = cIter->second; }
              if(nIter->second[side])
                { loaded++; }
            }
        }
      return loaded == gBlockSides.size();
    }
  else
    { return false; }
}

void World::updateAdjacent(const Point3i &wp)
{
  const Point3i cp = chunkPos(wp);
  const Point3i np = cp + sideDirection(Chunk::chunkEdge(Chunk::blockPos(wp)));
  const Point3i minP{std::min(cp[0], np[0]),
                     std::min(cp[1], np[1]),
                     std::min(cp[2], np[2]) };
  const Point3i maxP{std::max(cp[0], np[0]),
                     std::max(cp[1], np[1]),
                     std::max(cp[2], np[2]) };
  Point3i p;
  for(p[0] = minP[0]; p[0] <= maxP[0]; p[0]++)
    for(p[1] = minP[1]; p[1] <= maxP[1]; p[1]++)
      for(p[2] = minP[2]; p[2] <= maxP[2]; p[2]++)
        {
          auto iter = mChunks.find(Hash::hash(p));
          if(iter != mChunks.end() && iter->second)
            {
              iter->second->setDirty(true);
            }
        }
}

block_t* World::at(const Point3i &worldPos)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  Point3i cp = chunkPos(worldPos);
  auto iter = mChunks.find(Hash::hash(cp));
  if(iter != mChunks.end() && iter->second)
    { return iter->second->at(Chunk::blockPos(worldPos)); }
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
  if(chunk)
    {
      if((type == block_t::NONE || isSimpleBlock(type)) &&
         chunk->setBlock(Chunk::blockPos(worldPos), type) )
        {
          updateAdjacent(worldPos);
          chunk->setNeedSave(true);
          chunk->setPriority(true);
          return true;
        }
      else if(isFluidBlock(type) && data)
        {
          if(mFluids.set(worldPos, reinterpret_cast<Fluid*>(data)))
            {
              updateAdjacent(worldPos);
              chunk->setNeedSave(true);
              chunk->setPriority(true);
              return true;
            }
        }
      else
        {
          LOGDC(COLOR_RED, "No block set!");
        }
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

block_t* World::atBlock(const Point3i &wp, std::unordered_map<int32_t, Chunk*> &neighbors)
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
  if(!mMeshPool.running())
    {
      return;
    }
  std::unique_lock<std::mutex> lock(mMeshLock);
  mMeshCv.wait(lock, [this]{ return !mMeshPool.running() || mMeshQueue.size() > 0; });
  if(!mMeshPool.running())
    {
      return;
    }
  
  Chunk *next = mMeshQueue.back();
  mMeshQueue.pop_back();
  int32_t cHash = Hash::hash(next->pos());
  if(mMeshing.count(cHash) > 0)
    {
      mMeshing.erase(cHash);
      mMeshLock.unlock();
      updateChunkMesh(next);
    }
}

int World::getLighting(const Point3i &bp, const Point3f &vp, blockSide_t side,
                       std::unordered_map<int32_t, Chunk*> &neighbors )
{
  const int dim = sideDim(side); // dimension of face normal
  const int dim1 = (dim+1) % 3;  // dimension of edge 1
  const int dim2 = (dim+2) % 3;  // dimension of edge 2

  const int e1 = bp[dim1] + (int)vp[dim1] * 2 - 1;
  const int e2 = bp[dim2] + (int)vp[dim2] * 2 - 1;
  
  Point3i bi;
  bi[dim] = bp[dim] + sideSign(side);
  bi[dim1] = e1;
  bi[dim2] = e2;

  int lc = (isSimpleBlock(getBlock(bi, neighbors)) ? 1 : 0);
  
  bi[dim1] = bp[dim1];
  int le1 = (isSimpleBlock(getBlock(bi, neighbors)) ? 1 : 0);
  
  bi[dim1] = e1;
  bi[dim2] = bp[dim2];
  int le2 = (isSimpleBlock(getBlock(bi, neighbors)) ? 1 : 0);
  
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

void addChunkFace(MeshData &data, const Point3i &cp, bool meshed)
{
  Vector3f color = (meshed ? Vector3f{0,0,0} : Vector3f{1,0,0});
  int nVert = data.vertices().size();
  data.vertices().emplace_back((cp + Point3i{0,0,0})*Chunk::size, color, Point2i{0,0});
  data.vertices().emplace_back((cp + Point3i{0,1,0})*Chunk::size, color, Point2i{0,0});
  data.vertices().emplace_back((cp + Point3i{1,1,0})*Chunk::size, color, Point2i{0,0});
  data.vertices().emplace_back((cp + Point3i{1,0,0})*Chunk::size, color, Point2i{0,0});
  
  data.vertices().emplace_back((cp + Point3i{0,0,1})*Chunk::size, color, Point2i{0,0});
  data.vertices().emplace_back((cp + Point3i{0,1,1})*Chunk::size, color, Point2i{0,0});
  data.vertices().emplace_back((cp + Point3i{1,1,1})*Chunk::size, color, Point2i{0,0});
  data.vertices().emplace_back((cp + Point3i{1,0,1})*Chunk::size, color, Point2i{0,0});

  data.indices().push_back(nVert + 0);
  data.indices().push_back(nVert + 1);
  data.indices().push_back(nVert + 1);
  data.indices().push_back(nVert + 2);
  data.indices().push_back(nVert + 2);
  data.indices().push_back(nVert + 3);
  data.indices().push_back(nVert + 3);
  data.indices().push_back(nVert + 0);

  data.indices().push_back(nVert + 4);
  data.indices().push_back(nVert + 5);
  data.indices().push_back(nVert + 5);
  data.indices().push_back(nVert + 6);
  data.indices().push_back(nVert + 6);
  data.indices().push_back(nVert + 7);
  data.indices().push_back(nVert + 7);
  data.indices().push_back(nVert + 4);

  data.indices().push_back(nVert + 0);
  data.indices().push_back(nVert + 4);
  data.indices().push_back(nVert + 1);
  data.indices().push_back(nVert + 5);
  data.indices().push_back(nVert + 2);
  data.indices().push_back(nVert + 6);
  data.indices().push_back(nVert + 3);
  data.indices().push_back(nVert + 7);
  
}

MeshData World::makeChunkLineMesh()
{
  MeshData data;
  std::unordered_set<int32_t> mDone;

  int maxR = std::max(mLoadRadius[0], std::max(mLoadRadius[1], mLoadRadius[2]));
  const Point3i offset = mMaxChunk-1;

  Point3i cp;
  for(cp[0] = 0; cp[0] < mChunkDim[0]; cp[0]++)
    for(cp[1] = 0; cp[1] < mChunkDim[1]; cp[1]++)
      for(cp[2] = 0; cp[2] < mChunkDim[2]; cp[2]++)
        {
          int32_t cHash = Hash::hash(mMinChunk+cp);
          bool meshed;
          {
            std::lock_guard<std::mutex> lock(mRenderLock);
            auto rIter = mRenderMeshes.find(cHash);
            meshed = (rIter != mRenderMeshes.end());
          }
          std::lock_guard<std::mutex> lock(mChunkLock);
          auto cIter = mChunks.find(cHash);
          if(cIter != mChunks.end() && cIter->second)
            { addChunkFace(data, cp, meshed); }
        }
  return data;
}
void World::updateChunkMesh(Chunk *chunk)
{
  static const std::array<blockSide_t, 6> sides {{ blockSide_t::PX, blockSide_t::PY,
                                                   blockSide_t::PZ, blockSide_t::NX,
                                                   blockSide_t::NY, blockSide_t::NZ }};
  static const std::array<Point3i, 6> sideDirections {{ Point3i{1,0,0}, Point3i{0,1,0},
                                                        Point3i{0,0,1}, Point3i{-1,0,0},
                                                        Point3i{0,-1,0}, Point3i{0,0,-1} }};
  if(chunk->isEmpty())
    { return; }
  
  const Point3i cPos = chunk->pos();
  const int32_t cHash = Hash::hash(cPos);
  
  // get neighbors
  std::unordered_map<blockSide_t, Chunk*> sNeighbors;
  {
    sNeighbors = mNeighbors[cHash];
  }
  std::unordered_map<int32_t, Chunk*> neighbors;
  {
    for(auto &iter : mNeighbors[cHash])
      {
        if(!iter.second)
          {
            auto cIter = mChunks.find(Hash::hash(cPos + sideDirection(iter.first)));
            if(cIter != mChunks.end())
              neighbors[Hash::hash(cPos + sideDirection(iter.first))] = cIter->second;
          }
        else
          {
            neighbors[Hash::hash(cPos + sideDirection(iter.first))] = iter.second;
          }
      }
    neighbors[cHash] = chunk;
  }
  
  OuterShell *bounds = nullptr;
  auto iter = mChunkBoundaries.find(cHash);
  if(iter == mChunkBoundaries.end())
    {
      bounds = new OuterShell();
      mChunkBoundaries.emplace(cHash, bounds);
    }
  else
    { bounds = iter->second; }

  bounds->lock();
  bool empty = !bounds->calcShell(chunk, mNeighbors[cHash]);
  bounds->unlock();
  
  bool hasFluids = mFluids.setChunkBoundary(cHash, bounds);
  
  if(!empty)
    {
      MeshedChunk *mc = nullptr;
      {
        std::lock_guard<std::mutex> lock(mUnusedMCLock);
        if(mUnusedMC.size() > 0)
          {
            mc = mUnusedMC.front();
            mUnusedMC.pop();
          }
      }
      if(!mc)
        { mc = new MeshedChunk({cHash, MeshData()}); }
      else
        {
          mc->hash = cHash;
          mc->mesh.swap();
        }
      
      const Point3i minP{cPos[0]*Chunk::sizeX,
                         cPos[1]*Chunk::sizeY,
                         cPos[2]*Chunk::sizeZ};
      const Point3i maxP = minP + Chunk::size;

      bounds->lock();
      auto &chunkShell = bounds->getShell();
      for(auto &iter : chunkShell)
        {
          const int32_t hash = iter.first;
          const ActiveBlock &block = iter.second;
          
          const Point3i bp = Hash::unhash(hash);
          const Point3i vOffset = minP + bp;
          
          for(int i = 0; i < 6; i++)
            {
              if((bool)(block.sides & sides[i]))
                {
                  int vn = 0;
                  int sum0 = 0;
                  int sum1 = 0;
                  const unsigned int numVert = mc->mesh.vertices().size();
                  for(auto &v : faceVertices[sides[i]])
                    { // add vertices for this face
                      const int lighting = getLighting(vOffset, v.pos, sides[i], neighbors);
                      sum0 += (vn < 2) ? lighting : 0;
                      sum1 += (vn < 2) ? 0 : lighting;
                      
                      mc->mesh.vertices().emplace_back(vOffset + v.pos,
                                                       v.normal,
                                                       v.texcoord,
                                                       (int)block.block - 1,
                                                       (float)lighting / (float)4 );
                      vn++;
                    }
                  const std::array<unsigned int, 6> *orientedIndices = (sum1 > sum0 ?
                                                                        &flippedIndices :
                                                                        &faceIndices );
                  for(auto i : *orientedIndices)
                    { mc->mesh.indices().push_back(numVert + i); }
                }
            }
        }
      bounds->unlock();
      // pass to render thread
      std::lock_guard<std::mutex> lock(mRenderQueueLock);
      mRenderQueue.push(mc);
    }
}

void World::chunkLoadCallback(Chunk *chunk)
{
  //LOGD("CHUNK LOAD CALLBACK --> %d", (long)chunk);
  //  mChunks[Hash::hash(chunk->pos())] = chunk;
  mNumLoading--;
  std::lock_guard<std::mutex> lock(mLoadLock);
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
  mFluids.setRange(mMinChunk, mMaxChunk);
  mCenterDistIndex = 0;
}
void World::setRadius(const Vector3i &chunkRadius)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  LOGI("Setting chunk radius to (%d, %d, %d)", chunkRadius[0], chunkRadius[1], chunkRadius[2]);
  mLoadRadius = chunkRadius;
  mChunkDim = mLoadRadius * 2 + 1;
  mMinChunk = mCenter - mLoadRadius;
  mMaxChunk = mCenter + mLoadRadius;

  Vector3f bSize = mLoadRadius*Chunk::size;
  if(bSize[0] == 0) bSize[0] += Chunk::sizeX/2;
  if(bSize[1] == 0) bSize[1] += Chunk::sizeY/2;
  if(bSize[2] == 0) bSize[2] += Chunk::sizeZ/2;
  float minDim = std::min(bSize[0], std::min(bSize[1], bSize[2]));
  mDirScale = Vector3f{minDim/bSize[0],minDim/bSize[1],minDim/bSize[2]};
  mDirScale *= mDirScale;
  mFogStart = (minDim)*0.98f;
  mFogEnd = (minDim)*1.0f;
  mRadChanged = true;
  mCenterDistIndex = 0;
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
  Point3i chunkMin = mMinChunk*Chunk::size;
  Point3i chunkMax = (mMaxChunk+1)*Chunk::size;
  
  while((step[0] > 0 ? (pi[0] <= chunkMax[0]) : (pi[0] >= chunkMin[0])) &&
	(step[1] > 0 ? (pi[1] <= chunkMax[1]) : (pi[1] >= chunkMin[1])) &&
	(step[2] > 0 ? (pi[2] <= chunkMax[2]) : (pi[2] >= chunkMin[2])) )
    {
      if(!(pi[0] < chunkMin[0] || pi[1] < chunkMin[1] || pi[2] < chunkMin[2] ||
	   pi[0] > chunkMax[0] || pi[1] > chunkMax[1] || pi[2] > chunkMax[2] ))
	{
	  blockOut.type = getType(pi);
	  if(blockOut.type != block_t::NONE)
	    {
              blockOut.data = nullptr;//mFluids[cHash]->getData(posOut);
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
