#include "world.hpp"

#include "timing.hpp"
#include "shader.hpp"
#include "textureAtlas.hpp"
#include "chunkLoader.hpp"
#include "pointMath.hpp"
#include "meshBuffer.hpp"
#include "params.hpp"
#include "fluid.hpp"
#include "fluidChunk.hpp"
#include "meshRenderer.hpp"
#include "rayTracer.hpp"
#include "model.hpp"
#include "mesh.hpp"
#include <unistd.h>
#include <random>

World::World()
  : mLoader(new ChunkLoader(1, std::bind(&World::chunkLoadCallback, this, std::placeholders::_1))),
    mRenderer(new MeshRenderer()),
    mRayTracer(new RayTracer()),
    mCenterLoopCallback(std::bind(&World::checkChunkLoad, this, std::placeholders::_1))
{
  mFluids.setRange(mMinChunk, mMaxChunk);
  mRenderer->setFluids(&mFluids);
  //mRayTracer->setFluids(&mFluids);
}

World::~World()
{
  //mLoader->savePlayerPos(mPlayerStartPos);
  stop();
  mLoader->flush();
  delete mLoader;
  delete mRenderer;
  delete mRayTracer;
  
  for(auto &chunk : mChunks)
    {
      if(chunk.second)
        { delete chunk.second; }
    }
  while(mLoadQueue.size() > 0)
    {
      Chunk *chunk = mLoadQueue.front();
      mLoadQueue.pop_front();
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
  mRenderer->start();
}
void World::stop()
{
  mLoader->stop();
  mRenderer->stop();
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
  mRenderer->setMeshThreads(opt.meshThreads);
  
  mLoadRadius = opt.chunkRadius;
  mChunkDim = mLoadRadius * 2 + 1;
  mCenter = chunkPos(opt.playerPos);
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
  mRenderer->setFog(mFogStart, mFogEnd, mDirScale);
  mRayTracer->setFog(mFogStart, mFogEnd, mDirScale);
}

bool World::loadWorld(World::Options &opt)
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
      mPlayerStartPos = mLoader->getPlayerPos();
      opt.playerPos = mPlayerStartPos;
      updateInfo(opt);
      mPlayerReady = true;
      LOGI("Done loading world.");
      return true;
    }
}

bool World::createWorld(World::Options &opt, bool overwrite)
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
      
      mPlayerStartPos = playerStartPos();
      opt.playerPos = mPlayerStartPos;
      updateInfo(opt);
      LOGI("Done creating world.");
      std::cout << "PLAYER POS: " << mPlayerStartPos << ", CENTER: " << mCenter << "\n";
      return true;
    }
  else
    {
      if(overwrite)
        { LOGW("World exists -- no overwrite"); }
      return false;
    }
}

bool World::deleteWorld(const std::string &worldName)
{
  return mLoader->deleteWorld(worldName);
}

#define START_POS_CHUNK_RAD 4
#define START_SPACE_BLOCK_W 8

int World::getHeightAt(const Point2i &xy, bool *success)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  for(int z = 0; z < 4*Chunk::sizeZ; z++)
    {
      Point3i min{xy[0]-START_SPACE_BLOCK_W/2, xy[1]-START_SPACE_BLOCK_W/2, z};
      Point3i max{xy[0]+START_SPACE_BLOCK_W/2, xy[1]+START_SPACE_BLOCK_W/2, z+START_SPACE_BLOCK_W};
      Point3i p;
      bool open = true;
      for(p[0] = min[0]; p[0] <= max[0]; p[0]++)
        for(p[1] = min[1]; p[1] <= max[1]; p[1]++)
          for(p[2] = min[2]; p[2] <= max[2]; p[2]++)
            {
              auto cIter = mChunks.find(Hash::hash(chunkPos(p)));
              if(cIter == mChunks.end() || !cIter->second ||
                 cIter->second->getType(Chunk::blockPos(p)) != block_t::NONE )
                {
                  open = false;
                  break;
                }
            }
      if(open)
        {
          if(success)
            { *success = true; }
          return z;
        }
    }
  if(success)
    { *success = false; }
  return -100;
}

Point3f World::getStartPos()
{
  return Point3f({mPlayerStartPos[0] + 0.5f,
                  mPlayerStartPos[1] + 0.5f,
                  mPlayerStartPos[2] });
}

static auto startt = std::chrono::high_resolution_clock::now();
Point3i World::playerStartPos() const
{
  std::default_random_engine generator;
  generator.seed((std::chrono::high_resolution_clock::now() - startt).count());
  std::uniform_int_distribution<int> randX(-START_POS_CHUNK_RAD*Chunk::sizeX,
                                           START_POS_CHUNK_RAD*Chunk::sizeX );
  std::uniform_int_distribution<int> randY(-START_POS_CHUNK_RAD*Chunk::sizeY,
                                           START_POS_CHUNK_RAD*Chunk::sizeY );
  return Point3i({randX(generator), randY(generator), 0.0});
}

bool World::readyForPlayer()
{
  if(!mPlayerReady)
    {
      bool success = false;
      int z = getHeightAt(Point2f{mPlayerStartPos[0], mPlayerStartPos[1]}, &success);

      if(mNumLoaded > 64)
        {
          success = true;
          z = 0;
        }
      
      if(success)
        {
          mPlayerStartPos[2] = z + 1;
          LOGD("PLAYER START POS: %d,%d,%d", mPlayerStartPos[0], mPlayerStartPos[1], mPlayerStartPos[2]);
          
          mLoader->savePlayerPos(mPlayerStartPos);
          setCenter(chunkPos(mPlayerStartPos));
          mPlayerReady = true;
          
          // create platform below for player if necessary
          Point2i min{mPlayerStartPos[0]-START_SPACE_BLOCK_W/2,
                      mPlayerStartPos[1]-START_SPACE_BLOCK_W/2 };
          Point2i max{mPlayerStartPos[0]+START_SPACE_BLOCK_W/2,
                      mPlayerStartPos[1]+START_SPACE_BLOCK_W/2 };
          Point3i p;
          p[2] = z;
          for(p[0] = min[0]; p[0] <= max[0]; p[0]++)
            for(p[1] = min[1]; p[1] <= max[1]; p[1]++)
                {
                  //setBlock(p, block_t::NONE);
                  setBlock(p, block_t::STONE);
                }

        }
    }
      
  
  return mPlayerReady;
}

void World::setPlayerPos(const Point3i &ppos)
{
  mLoader->savePlayerPos(ppos);
}

void World::setDebug(bool debug)
{ mDebug = debug; }
void World::setFluidSim(bool on)
{ mSimFluids = on; }
void World::setFluidEvap(float rate)
{ mEvapRate = rate; }
void World::setRaytracing(bool on)
{ mRaytrace = on; }
void World::clearFluids()
{
  LOGDC(COLOR_YELLOW, "CLEARING FLUIDS!!");
  mFluids.clear();
  mRenderer->clearFluids();
  //mRayTracer->clearFluids();
}

int World::numMeshed()
{ return mRenderer->numMeshed(); }

bool World::chunkIsLoading(const Point3i &cp)
{
  auto iter = mChunks.find(Hash::hash(cp));
  return (iter != mChunks.end() && !iter->second);
}
bool World::chunkIsLoaded(const Point3i &cp)
{
  auto iter = mChunks.find(Hash::hash(cp));
  return (iter != mChunks.end() && iter->second);
}
bool World::chunkIsMeshed(const Point3i &cp)
{
  auto iter = mChunks.find(Hash::hash(cp));
  return (iter != mChunks.end() && iter->second && mRenderer->isMeshed(Hash::hash(cp)));
}
bool World::chunkIsEmpty(const Point3i &cp)
{
  auto iter = mChunks.find(Hash::hash(cp));
  return (iter != mChunks.end() && iter->second && iter->second->isEmpty());
}

void World::reset()
{
  clearFluids();

  for(auto &chunk : mChunks)
    {
      if(chunk.second)
        { delete chunk.second; }
    }
  mChunks.clear();
  while(mLoadQueue.size() > 0)
    {
      Chunk *chunk = mLoadQueue.front();
      mLoadQueue.pop_front();
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
  mResetGL = true;
  mPlayerReady = false;
  mNumLoaded = 0;
  mNumLoading = 0;
  mCenterDistIndex = 0;
}

bool World::initGL(QObject *qParent)
{
  if(!mInitialized)
    {
      if(!mRenderer->initGL(qParent))
        { return false; }
      else
        { mRenderer->setFog(mFogStart, mFogEnd, mDirScale); }
      
      if(!mRayTracer->initGL(qParent))
        {
          mRenderer->cleanupGL();
          return false;
        }
      else
        { mRayTracer->setFog(mFogStart, mFogEnd, mDirScale); }
      
      mChunkLineShader = new Shader(qParent);
      if(!mChunkLineShader->loadProgram("./shaders/chunkLine.vsh", "./shaders/chunkLine.fsh",
                                        {"posAttr", "normalAttr",  "texCoordAttr"},
                                        {"pvm", "camPos", "fogStart", "fogEnd", "dirScale"} ))
        {
          LOGE("Chunk line shader failed to load!");
          delete mChunkLineShader;
          mChunkLineShader = nullptr;
          mRenderer->cleanupGL();
          mRayTracer->cleanupGL();
          return false;
        }
      else
        {
          mChunkLineShader->bind();
          mChunkLineShader->setUniform("fogStart", mFogStart);
          mChunkLineShader->setUniform("fogEnd", mFogEnd);
          mChunkLineShader->setUniform("dirScale", mDirScale);
          mChunkLineShader->release();

          mChunkLineMesh = new cMeshBuffer();
          mChunkLineMesh->initGL(mChunkLineShader);
          mChunkLineMesh->setMode(GL_LINES);
        }

      
      mFrustumShader = new Shader(qParent);
      if(!mFrustumShader->loadProgram("./shaders/frustumView.vsh", "./shaders/frustumView.fsh",
                                        {"posAttr", "normalAttr", "texCoordAttr"},
                                        {"pvm", "camPos", "fogStart", "fogEnd", "dirScale"} ))
        {
          LOGE("Chunk line shader failed to load!");
          delete mChunkLineShader;
          mChunkLineShader = nullptr;
          delete mFrustumShader;
          mFrustumShader = nullptr;
          mRenderer->cleanupGL();
          mRayTracer->cleanupGL();
          return false;
        }
      else
        {
          mFrustumShader->bind();
          mFrustumShader->setUniform("fogStart", mFogStart);
          mFrustumShader->setUniform("fogEnd", mFogEnd);
          mFrustumShader->setUniform("dirScale", mDirScale);
          mFrustumShader->release();
          
          mFrustumMesh = new cMeshBuffer();
          mFrustumMesh->initGL(mFrustumShader);
        }

      mInitialized = true;
    }
  return true;
}
void World::cleanupGL()
{
  if(mInitialized)
    {
      mRenderer->cleanupGL();
      mRayTracer->cleanupGL();
      delete mChunkLineMesh;
      mChunkLineMesh = nullptr;
      delete mChunkLineShader;
      mChunkLineShader = nullptr;
      delete mFrustumMesh;
      mFrustumMesh = nullptr;
      delete mFrustumShader;
      mFrustumShader = nullptr;
      
      mInitialized = false;
    }
}

void World::render(const Matrix4 &pvm)
{
  if(mRaytrace)
    {
      mRayTracer->render(pvm, mCamPos);
    }
  else
    {
      mRenderer->render(pvm, mCamPos, mResetGL);
    }

  mResetGL = false;

  if(mDebug)
    {
      // render all loaded chunks
      mChunkLineShader->bind();
      mChunkLineMesh->uploadData(makeChunkLineMesh());
      mChunkLineShader->setUniform("pvm", pvm*matTranslate(mMinChunk[0]*Chunk::sizeX,
                                                           mMinChunk[1]*Chunk::sizeY,
                                                           mMinChunk[2]*Chunk::sizeZ ));
      mChunkLineShader->setUniform("camPos", mCamPos);
      if(mRadChanged)
        {
          mChunkLineShader->setUniform("fogStart", mFogStart);
          mChunkLineShader->setUniform("fogEnd", mFogEnd);
          mChunkLineShader->setUniform("dirScale", mDirScale);
          
          mFrustumShader->bind();
          mFrustumShader->setUniform("fogStart", mFogStart);
          mFrustumShader->setUniform("fogEnd", mFogEnd);
          mFrustumShader->setUniform("dirScale", mDirScale);
          mFrustumShader->release();
          mRadChanged = false;
        }
      mChunkLineShader->bind();
      mChunkLineMesh->render();
      mChunkLineShader->release();
      
      // render all loaded chunks
      mFrustumShader->bind();
      mFrustumShader->setUniform("pvm", pvm);//*matTranslate();
      mFrustumShader->setUniform("camPos", mCamPos);
      mFrustumMesh->uploadData(mRenderer->makeFrustumMesh());
      mFrustumMesh->render();
      mFrustumShader->release();
    }
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
      mLoader->load(chunk);
      return true;
    }
    
  return false;
}

void World::update()
{
  auto start = std::chrono::high_resolution_clock::now();
  std::vector<std::pair<hash_t, Chunk*>> unload;
  {
    std::lock_guard<std::mutex> lock(mChunkLock);
    // find any chunks that are out of range to unload
    for(auto &iter : mChunks)
      {
        if(!pointInRange(Hash::unhash(iter.first), mMinChunk, mMaxChunk))
          { unload.push_back(iter); }
      }
  }
  // unload the chunks
  for(auto uIter : unload)
    {
      hash_t cHash = uIter.first;
      Chunk *chunk = uIter.second;
      if(chunk)
        {
          mRenderer->unload(cHash);
          mRayTracer->unload(cHash);
          
          if(chunk->needsSave())
            {
              chunk->setNeedSave(false);
              mLoader->save(chunk);
            }

          for(auto &side : gBlockSides)
            { // remove this chunk from neighbors
              Chunk *n = chunk->getNeighbor(side);
              if(n)
                {
                  n->setReady(false);
                  n->unsetNeighbor(oppositeSide(side));
                  n->unsetNeighbor(side);
                }
            }
          mNumLoaded--;
          mUnusedChunks.push(chunk);
        }
      
      std::lock_guard<std::mutex> lock(mChunkLock);
      mChunks.erase(cHash);
    }
  
  { // start loading chunks in range
    const std::vector<Point3i> &distPoints = getCenterDistPoints(mCenter, mLoadRadius);
    for(int i = 0; i < distPoints.size(); i++)
      {
        const Point3i cp = mCenter + distPoints[i];//{x,y,z};
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

            {
              std::lock_guard<std::mutex> lock(mChunkLock);
              mChunks.emplace(cHash, nullptr);
            }
            mLoader->load(chunk);
            if(mNumLoading++ > mLoader->numThreads()*4)
              { break; }
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
            mLoadQueue.pop_front();
          }
        else
          { break; }
      }
      if(chunk && pointInRange(chunk->pos(), mMinChunk, mMaxChunk))
        {
          const hash_t cHash = Hash::hash(chunk->pos());
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
                    const hash_t nHash = Hash::hash(chunk->pos() + sideDirection(side));
                    auto nIter = mChunks.find(nHash);
                    if(nIter != mChunks.end() && nIter->second)
                      {
                        chunk->setNeighbor(side, nIter->second);
                        nIter->second->setNeighbor(oppositeSide(side), chunk);
                      }
                  }
                  
                if(chunk->isEmpty())
                  { // empty mesh
                    chunk->setDirty(true);
                    chunk->setReady(true);
                  }
                else
                  {
                    chunk->setDirty(true);
                    chunk->setReady(false);
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
        const hash_t cHash = iter.first;
        if(chunk && !chunk->isEmpty())
          {
            const Point3i cp = chunk->pos();
            if(cp[0] > mMinChunk[0] || cp[0] < mMaxChunk[0] &&
               cp[1] > mMinChunk[1] || cp[1] < mMaxChunk[1] &&
               cp[2] > mMinChunk[2] || cp[2] < mMaxChunk[2] )
              {
                bool nLoaded = neighborsLoaded(cHash, chunk);
                if(!chunk->isReady() && nLoaded)
                  {
                    chunk->setReady(true);
                    chunk->setDirty(true);
                  }
                // else if(chunk->isReady() && !nLoaded)
                //   {
                //     chunk->setReady(false);
                //     mRenderer->unload(iter.first);
                //     mRayTracer->unload(iter.first);
                //   }
            
                if(chunk->isReady() && chunk->isDirty())
                  { // mesh needs updating
                    chunk->setDirty(false);
                    mRenderer->load(chunk, mCenter);
                    mRayTracer->load(iter.first, chunk);
                  }
              }
          }
      }
  }
  
  std::vector<Chunk*> saveChunks;
  { // save any chunks that have been modified
    std::lock_guard<std::mutex> lock(mChunkLock);
    for(auto &iter : mChunks)
      {
        if(iter.second && iter.second->needsSave())
          { saveChunks.push_back(iter.second); }
      }
  }
  for(auto &chunk : saveChunks)
    {
      mLoader->save(chunk);
      chunk->setNeedSave(false);
    }
  double t = (std::chrono::high_resolution_clock::now() - start).count();

  std::lock_guard<std::mutex> lock(mTimingLock);
  mMeshTime += t;
  mMeshNum++;
  if(mMeshNum >= 16)
    {
      //LOGD("Avg Update Time: %f", mMeshTime / mMeshNum / 1000000000.0);
      mMeshTime = 0.0;
      mMeshNum = 0;
    }
}

void World::step()
{
  if(!mSimFluids)
    { return; }

  std::unordered_map<hash_t, bool> updates = mFluids.step(mEvapRate);
  
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

  for(auto &cIter : mChunks)
    {
      if(cIter.second)
        { cIter.second->update(); }
    }
}

blockSide_t World::getEdges(hash_t hash)
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

bool World::neighborsLoaded(hash_t hash, Chunk *chunk)
{
  const blockSide_t edgeSides = getEdges(hash);
  int loaded = 0;
  for(auto side : gBlockSides)
    {
      Chunk *neighbor = chunk->getNeighbor(side);
      if(((edgeSides & side) != blockSide_t::NONE) || neighbor)
        { loaded++; }
      else
        {
          auto cIter = mChunks.find(Hash::hash(chunk->pos() + sideDirection(side)));
          if(cIter != mChunks.end())
            { chunk->setNeighbor(side, cIter->second); }
          if(chunk->getNeighbor(side))
            { loaded++; }
        }
    }
  return loaded == gBlockSides.size();
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
          mFluids.set(worldPos, nullptr);
          return true;
        }
      else if(isFluidBlock(type) && data)
        {
          if(mFluids.set(worldPos, reinterpret_cast<Fluid*>(data)))
            {
              updateAdjacent(worldPos);
              chunk->setNeedSave(true);
              return true;
            }
        }
      else if(isComplexBlock(type) && data)
        {
          if(chunk->setComplex(Chunk::blockPos(worldPos), {type, data}))
            {
              updateAdjacent(worldPos);
              chunk->setNeedSave(true);
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

block_t World::getBlock(const Point3i &wp)
{
  auto cIter = mChunks.find(Hash::hash(chunkPos(wp)));
  if(cIter != mChunks.end() && cIter->second)
    { return cIter->second->getType(Chunk::blockPos(wp)); }
  else
    { return block_t::NONE; }
}

block_t* World::atBlock(const Point3i &wp)
{
  auto cIter = mChunks.find(Hash::hash(chunkPos(wp)));
  if(cIter != mChunks.end() && cIter->second)
    { return cIter->second->at(Chunk::blockPos(wp)); }
  else
    { return nullptr; }
}

void addChunkFace(MeshData &data, Chunk *chunk, const Point3i &cp, bool meshed)
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

  // add edge connections
  nVert = data.vertices().size();
  for(int i = 0; i < 6; i++)
    {
      Vector3f offset = sideDirection((blockSide_t)(1 << i));
      for(int d = 0; d < 3; d++)
        {
          if(offset[d] == 0)
            { offset[d] = Chunk::size[d]/2; }
          else if(offset[d] > 0)
            { offset[d] = Chunk::size[d] - 8; }
          else
            { offset[d] = 8; }
        }

      data.vertices().emplace_back(cp*Chunk::size + offset,
                                   Vector3f{1.0f, 1.0f, 1.0f}, Point2i{0,0} );
    }
  for(int i = 0; i < 6; i++)
    {
      for(int j = i+1; j < 6; j++)
    {
      if(chunk->edgesConnected((blockSide_t)(1 << i), (blockSide_t)(1 << j)))
        {
         data.indices().push_back(nVert + i);
         data.indices().push_back(nVert + j);
        }
    }
  }
}

MeshData World::makeChunkLineMesh()
{
  MeshData data;
  std::unordered_set<hash_t> mDone;

  int maxR = std::max(mLoadRadius[0], std::max(mLoadRadius[1], mLoadRadius[2]));
  const Point3i offset = mMaxChunk-1;

  Point3i cp;
  for(cp[0] = 0; cp[0] < mChunkDim[0]; cp[0]++)
    for(cp[1] = 0; cp[1] < mChunkDim[1]; cp[1]++)
      for(cp[2] = 0; cp[2] < mChunkDim[2]; cp[2]++)
        {
          hash_t cHash = Hash::hash(mMinChunk+cp);
          bool meshed = mRenderer->isMeshed(cHash);
          
          std::lock_guard<std::mutex> lock(mChunkLock);
          auto cIter = mChunks.find(cHash);
          if(cIter != mChunks.end() && cIter->second)
            { addChunkFace(data, cIter->second, cp, meshed); }
        }
  
  return data;
}

void World::chunkLoadCallback(Chunk *chunk)
{
  mNumLoading--;
  std::lock_guard<std::mutex> lock(mLoadLock);
  mLoadQueue.push_back(chunk);

  /*
    { // start loading chunks in range
    const std::vector<Point3i> &distPoints = getCenterDistPoints(mCenter, mLoadRadius);
    if(mCenterDistIndex < distPoints.size())
    {
    for(int i = mCenterDistIndex; i < distPoints.size(); i++, mCenterDistIndex++)
    {
    const Point3i cp = mCenter + distPoints[i];
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

    {
    std::lock_guard<std::mutex> lock(mChunkLock);
    mChunks.emplace(cHash, nullptr);
    }
    mLoader->load(chunk);
    if(++mNumLoading >= mLoader->numThreads()*16)
    { break; }
    }
    }
    }
    }
  */
}


void World::setCenter(const Point3i &chunkCenter)
{
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
  mRenderer->setCenter(mCenter);
  mRayTracer->setCenter(mCenter);
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
  mRenderer->setFog(mFogStart, mFogEnd, mDirScale);
  mRayTracer->setFog(mFogStart, mFogEnd, mDirScale);
  
  mCenterDistIndex = 0;
}
Point3i World::getCenter() const
{ return mCenter; }

void World::setFrustum(Frustum *frustum)
{
  mRenderer->setFrustum(frustum);
  mRayTracer->setFrustum(frustum);
}
void World::setFrustumCulling(bool on)
{
  mRenderer->setFrustumCulling(on);
  mRayTracer->setFrustumCulling(on);
}
void World::pauseFrustumCulling()
{
  mRenderer->pauseFrustumCulling();
  mRayTracer->pauseFrustum();
}

void World::setScreenSize(const Point2i &size)
{ mRayTracer->setScreenSize(size); }

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
          hash_t cHash = Hash::hash(pi);
	  FluidChunk *fc = mFluids.getChunk(cHash);
          if(fc)
            {
              Fluid *f = fc->at(Chunk::blockPos(pi));
              if(f)
                {
                  blockOut.type = f->type;
                  if(blockOut.type != block_t::NONE)
                    {
                      blockOut.data = f;
                      posOut = pi;
                      return true;
                    }
                }
            }
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
