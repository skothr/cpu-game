#include "world.hpp"
#include "params.hpp"
#include "shader.hpp"
#include "pointMath.hpp"
#include "meshBuffer.hpp"
#include "mesh.hpp"
#include "fluid.hpp"
#include "fluidChunk.hpp"
#include "meshRenderer.hpp"
#include "rayTracer.hpp"

#include "chunkLoader.hpp"
#include "chunkVisualizer.hpp"

#include <unistd.h>
#include <random>


#define FOG_START 0.9
#define FOG_END 1.1


World::World()
  : mLoader(new ChunkLoader(1, std::bind(&World::chunkLoadCallback,
                                         this, std::placeholders::_1 ))),
    mRenderer(new MeshRenderer()), mRayTracer(new RayTracer()),
    mVisualizer(new ChunkVisualizer(Vector2i{512, 512}))
{
  mChunkMap.setLoader(mLoader);
  mFluids.setRange(mMinChunk, mMaxChunk);
  mRenderer->setFluids(&mFluids);
  mRenderer->setMap(&mChunkMap);
  //mRayTracer->setFluids(&mFluids);
}

World::~World()
{
  stop();
  delete mRenderer;
  delete mRayTracer;
  delete mLoader;
}

void World::start()
{
  mLoader->start();
  mRenderer->startMeshing();
}
void World::stop()
{
  mLoader->stop();
  mRenderer->stopMeshing();
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
  auto unload = mChunkMap.unloadOutside(mMinChunk-1, mMaxChunk+1);
  for(auto hash : unload)
    {
      mRenderer->unload(hash);
      //mRayTracer->unload(hash);
      mVisualizer->unload(hash);
    }

  Vector3f bSize = mLoadRadius*Chunk::size;
  if(bSize[0] == 0) bSize[0] += Chunk::sizeX/2;
  if(bSize[1] == 0) bSize[1] += Chunk::sizeY/2;
  if(bSize[2] == 0) bSize[2] += Chunk::sizeZ/2;
  float minDim = std::min(bSize[0], std::min(bSize[1], bSize[2]));
  mDirScale = Vector3f{minDim/bSize[0],minDim/bSize[1],minDim/bSize[2]};
  mDirScale *= mDirScale;
  mFogStart = (minDim)*FOG_START;
  mFogEnd = (minDim)*FOG_END;
  mRenderer->setFog(mFogStart, mFogEnd, mDirScale);
  //mRayTracer->setFog(mFogStart, mFogEnd, mDirScale);
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
  //std::lock_guard<std::mutex> lock(mChunkLock);
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
              ChunkPtr chunk = mChunkMap[chunkPos(p)];
              if(!chunk || chunk->getType(Chunk::blockPos(p)) != block_t::NONE )
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
  return Point3f({(float)mPlayerStartPos[0] + (mPlayerStartPos[0] >= 0 ? 0.5f : -0.5f),
                  (float)mPlayerStartPos[1] + (mPlayerStartPos[1] >= 0 ? 0.5f : -0.5f),
                  (float)mPlayerStartPos[2] + 0.5f });
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

      if(mChunkMap.numLoaded() > 64)
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
                  setBlock(p, block_t::NONE);
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

int World::numLoaded() const
{ return mChunkMap.numLoaded(); }

bool World::chunkIsLoading(const Point3i &cp)
{
  return mChunkMap.isLoading(cp);
}
bool World::chunkIsLoaded(const Point3i &cp)
{
  return mChunkMap.isLoaded(cp);
}
bool World::chunkIsMeshed(const Point3i &cp)
{
  return (mRenderer->isMeshed(Hash::hash(cp)));
}
bool World::chunkIsMeshing(const Point3i &cp)
{
  return (mRenderer->isMeshing(Hash::hash(cp)));
}
bool World::chunkIsEmpty(const Point3i &cp)
{
  ChunkPtr chunk = mChunkMap[cp];
  return (chunk && chunk->isEmpty());
}
bool World::chunkIsReady(const Point3i &cp)
{
  /*
  ChunkPtr chunk = mChunkMap[cp];
  if(chunk)
    { return chunk->isReady(); }
  else
    { return false; }
  */
  return mChunkMap.isReady(cp);
}
bool World::chunkIsVisible(const Point3i &cp)
{
  return mRenderer->isVisible(Hash::hash(cp));
}

void World::reset()
{
  clearFluids();
  mChunkMap.clear();
  
  mResetGL = true;
  mPlayerReady = false;
}

bool World::initGL(QObject *qParent)
{
  if(!mInitialized)
    {
      if(!mRenderer->initGL(qParent))
        { return false; }
      else
        { mRenderer->setFog(mFogStart, mFogEnd, mDirScale); }
      
      // if(!mRayTracer->initGL(qParent))
      //   {
      //     mRenderer->cleanupGL();
      //     return false;
      //   }
      // else
      //   { mRayTracer->setFog(mFogStart, mFogEnd, mDirScale); }

      if(!mVisualizer->initGL(qParent))
        {
          mRenderer->cleanupGL();
          //mRayTracer->cleanupGL();
          return false;
        }
      
      mChunkLineShader = new Shader(qParent);
      if(!mChunkLineShader->loadProgram("./shaders/chunkLine.vsh", "./shaders/chunkLine.fsh",
                                        {"posAttr", "normalAttr",  "texCoordAttr"},
                                        {"pvm", "camPos", "fogStart", "fogEnd", "dirScale"} ))
        {
          LOGE("Chunk line shader failed to load!");
          delete mChunkLineShader;
          mChunkLineShader = nullptr;
          mVisualizer->cleanupGL();
          mRenderer->cleanupGL();
          //mRayTracer->cleanupGL();
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
          mVisualizer->cleanupGL();
          mRenderer->cleanupGL();
          //mRayTracer->cleanupGL();
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
      //mRayTracer->cleanupGL();
      mVisualizer->cleanupGL();
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
      //mRayTracer->render(pvm, mCamPos);
    }
  else
    {
      mRenderer->render(pvm, mCamPos, mResetGL);
    }
  
  //mVisualizer->render();
  mResetGL = false;
  
  if(mRadChanged)
    {
      mChunkLineShader->bind();
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

  if(mDebug)
    {
      // render all loaded chunks
      glLineWidth(10.0);
      mChunkLineShader->bind();
      mChunkLineShader->setUniform("pvm", pvm*matTranslate(mMinChunk[0]*Chunk::sizeX,
                                                           mMinChunk[1]*Chunk::sizeY,
                                                           mMinChunk[2]*Chunk::sizeZ ));
      mChunkLineMesh->uploadData(makeChunkLineMesh());
      mChunkLineShader->setUniform("camPos", mCamPos);
      mChunkLineMesh->render();
      mChunkLineShader->release();
      //glLineWidth(1.0);
      
    }
  // render all loaded chunks
  if(mFrustumPaused)
    {
      mFrustumShader->bind();
      mFrustumShader->setUniform("pvm", pvm);
      mFrustumShader->setUniform("camPos", mCamPos);
      mFrustumMesh->uploadData(mRenderer->makeFrustumMesh());
      mFrustumMesh->render();
      mFrustumShader->release();
    }
}

#define LOAD_PER_UPDATE 512
#define SAVES_PER_UPDATE 4

void World::update()
{
  auto start = std::chrono::high_resolution_clock::now();
  mChunkMap.update();

  /*
  Point3i p;
  int i = 0;
  for(p[0] = mMinChunk[0]-1; p[0] <= mMaxChunk[0]+1; p[0]++)
    for(p[1] = mMinChunk[1]-1; p[1] <= mMaxChunk[1]+1; p[1]++)
      for(p[2] = mMinChunk[2]-1; p[2] <= mMaxChunk[2]+1; p[2]++)
        {
          if(!mChunkMap.isLoading(p) && !mChunkMap.isLoaded(p))
            {
              mChunkMap.load(p);
            }
        }
  
  TreeNode tree = mChunkMap.getTree();
  std::queue<TreeNode*> steps;
  steps.push(&tree);

  while(steps.size() > 0)
    {
      TreeNode *node = steps.front();
      steps.pop();

      ChunkPtr chunk = node->chunk;
      hash_t hash = node->hash;
      
      bool loaded = mChunkMap.isLoaded(node->pos);
      if(!mChunkMap.isLoading(node->pos) && !loaded)
        {
          mChunkMap.load(node->pos);
          //if(++i >= LOAD_PER_UPDATE)
          //{ break; }
        }
      else if(loaded)
        {
          // TODO: Optimize (currently calculating same hash a few times)
          if(chunk)
            {
              // if(mRenderer->isMeshed(hash))
              //   { mVisualizer->setState(hash, ChunkState::MESHED); }
              // else if(chunk->isReady())
              //   { mVisualizer->setState(hash, ChunkState::READY); }
              // else
              //   { mVisualizer->setState(hash, ChunkState::LOADED); }
              
              if(chunk->isReady() && chunk->isDirty())
                { // mesh needs updating
                  bool priority = chunk->isPriority();
                  chunk->setDirty(false);
                  chunk->setPriority(false);
                  LOGD("MESHING CHUNK!!");
                  mRenderer->load(chunk, mCenter, priority);
                  mRayTracer->load(hash, chunk);
                }

              for(auto &c : node->children)
                { steps.push(&c); }

              // save chunk
              if(chunk->needsSave())
                {
                  mLoader->save(chunk);
                  chunk->setNeedSave(false);
                }
            }
        }
      // else
      //   {
      //     mVisualizer->setState(hash, ChunkState::LOADING);
      //   }
    }
*/

  Point3i p;
  int i = 0;
  int saves = 0;
  for(p[0] = mMinChunk[0]-1; p[0] <= mMaxChunk[0]+1; p[0]++)
    for(p[1] = mMinChunk[1]-1; p[1] <= mMaxChunk[1]+1; p[1]++)
      for(p[2] = mMinChunk[2]-1; p[2] <= mMaxChunk[2]+1; p[2]++)
        {
          hash_t hash = Hash::hash(p);
          bool loaded = mChunkMap.isLoaded(p);
          bool ready = mChunkMap.isReady(p);
          if(!mChunkMap.isLoading(p) && !loaded)
            {
              mChunkMap.load(p);
              if(++i >= LOAD_PER_UPDATE)
              { break; }
            }
          else if(loaded)
            {
              ChunkPtr chunk = mChunkMap[hash];
              // TODO: Optimize (currently calculating same hash a few times)
              if(chunk)
                {
                  // if(mRenderer->isMeshed(Hash::hash(p)))
                  //   { mVisualizer->setState(hash, ChunkState::MESHED); }
                  // else if(chunk->isReady())
                  //   { mVisualizer->setState(hash, ChunkState::READY); }
                  // else
                  //   { mVisualizer->setState(hash, ChunkState::LOADED); }
              
                  if(chunk->isDirty())
                    { // mesh needs updating
                      if(ready)
                        {
                          chunk->setDirty(false);
                          bool priority = chunk->isPriority();
                          chunk->setPriority(false);
                          mRenderer->load(chunk, mCenter, priority);
                          //mRayTracer->load(hash, chunk);
                        }
                      else if(chunk->isUnloaded())
                        {
                          chunk->setDirty(false);
                          chunk->setUnloaded(false);
                          mRenderer->unload(hash);
                        }
                    }

                  // save chunk
                  if(chunk->needsSave())
                    {
                      mLoader->save(chunk);
                      chunk->setNeedSave(false);
                      if(++saves >= SAVES_PER_UPDATE)
                        { break; }
                    }
                }
            }
          // else
          //   {
          //     mVisualizer->setState(hash, ChunkState::LOADING);
          //   }
        }
  
  
  mUpdateTime += (std::chrono::high_resolution_clock::now() - start).count();
  mUpdateCount++;
  if(mUpdateCount >= 2)
    {
      LOGD("Update Time: %f", mUpdateTime / 2 / 1000000000.0f);
      mUpdateTime = 0.0;
      mUpdateCount = 0;
    }

  if(mVisualizerRotate.max() != 0.0f || mVisualizerRotate.min() != 0.0f)
    {
      Vector3f eyeAngle = mVisualizer->getEyeAngle();
      mVisualizer->setEyeAngle(eyeAngle + mVisualizerRotate);
    }
}

void World::step()
{
  if(!mSimFluids)
    { return; }

  std::unordered_map<hash_t, bool> updates = mFluids.step(mEvapRate);
  
  //std::lock_guard<std::mutex> lock(mChunkLock);
  for(auto &c : updates)
    {
      if(c.second)
        {
          ChunkPtr chunk = mChunkMap[Hash::unhash(c.first)]; // TOO: Remove unhash
          if(chunk)
            {
              chunk->update();
              chunk->setDirty(true);
              chunk->setPriority(true);
            }
        }
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

block_t* World::at(const Point3i &worldPos)
{
  ChunkPtr chunk = mChunkMap[chunkPos(worldPos)];
  if(chunk)
    { return chunk->at(Chunk::blockPos(worldPos)); }
  else
    { return nullptr; }
}
block_t World::getType(const Point3i &worldPos)
{
  ChunkPtr chunk = mChunkMap[chunkPos(worldPos)];
  if(chunk)
    { return chunk->getType(Chunk::blockPos(worldPos)); }
  else
    { return block_t::NONE; }
}

ChunkPtr World::getChunk(const Point3i &worldPos)
{
  return mChunkMap[chunkPos(worldPos)];
}

void World::rotateVisualizer(const Vector3f &angle)
{
  mVisualizerRotate += angle;
}

bool World::setBlock(const Point3i &worldPos, block_t type, BlockData *data)
{
  //std::lock_guard<std::mutex> lock(mChunkLock);
  Point3i cp = chunkPos(worldPos);
  ChunkPtr chunk = mChunkMap[cp];
  if(chunk)
    {
      Point3i bp = Chunk::blockPos(worldPos);
      if((type == block_t::NONE || isSimpleBlock(type)) &&
         chunk->setBlock(Chunk::blockPos(worldPos), type) )
        {
          mChunkMap.updateAdjacent(chunkPos(worldPos), Chunk::chunkEdge(bp));
          chunk->updateConnected();
          chunk->setNeedSave(true);
          chunk->setPriority(true);
          mFluids.set(worldPos, nullptr);
          return true;
        }
      else if(isFluidBlock(type) && data)
        {
          if(mFluids.set(worldPos, reinterpret_cast<Fluid*>(data)))
            {
              mChunkMap.updateAdjacent(chunkPos(worldPos), Chunk::chunkEdge(bp));
              chunk->setNeedSave(true);
              chunk->setPriority(true);
              return true;
            }
        }
      else if(isComplexBlock(type) && data)
        {
          if(chunk->setComplex(Chunk::blockPos(worldPos), {type, data}))
            {
              mChunkMap.updateAdjacent(chunkPos(worldPos), Chunk::chunkEdge(bp));
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


bool World::setSphere(const Point3i &center, int rad, block_t type, BlockData *data)
{
  Point3i minP = center - rad;
  Point3i maxP = center + rad;
  Point3i cMin = chunkPos(minP);
  Point3i cMax = chunkPos(maxP);
  Point3i minB = Chunk::blockPos(minP);
  Point3i maxB = Chunk::blockPos(maxP);
  std::unordered_map<ChunkPtr, blockSide_t> chunkUpdates;
  
  Point3i cp;
  for(cp[0] = cMin[0]; cp[0] <= cMax[0]; cp[0]++)
    for(cp[1] = cMin[1]; cp[1] <= cMax[1]; cp[1]++)
      for(cp[2] = cMin[2]; cp[2] <= cMax[2]; cp[2]++)
        {
          ChunkPtr chunk = mChunkMap[cp];
          if(chunk)
            {
              Point3i wp = cp*Chunk::size;
              Point3i bmin{cp[0] == cMin[0] ? minB[0] : 0,
                           cp[1] == cMin[1] ? minB[1] : 0,
                           cp[2] == cMin[2] ? minB[2] : 0};
              Point3i bmax{cp[0] == cMax[0] ? maxB[0] : Chunk::sizeX-1,
                           cp[1] == cMax[1] ? maxB[1] : Chunk::sizeY-1,
                           cp[2] == cMax[2] ? maxB[2] : Chunk::sizeZ-1};

              bool changed = false;
              Point3i bp;
              for(bp[0] = bmin[0]; bp[0] <= bmax[0]; bp[0]++)
                for(bp[1] = bmin[1]; bp[1] <= bmax[1]; bp[1]++)
                  for(bp[2] = bmin[2]; bp[2] <= bmax[2]; bp[2]++)
                    {
                      Point3i diff =  wp + bp - center;
                      float dist = sqrt(diff.dot(diff));
                      if(dist <= rad)
                        { changed = true; }
                      if(dist < rad)
                        {
                          if((type == block_t::NONE || isSimpleBlock(type)) &&
                             chunk->setBlock(bp, type) )
                            {
                              mFluids.set(bp, nullptr);
                            }
                          else if(isFluidBlock(type) && data)
                            {
                              mFluids.set(bp, reinterpret_cast<Fluid*>(data));
                            }
                        }
                    }
              if(changed)
                {
                  chunkUpdates.emplace(chunk, (((cp[0] == cMin[0] && bmin[0] == 0 ? blockSide_t::NX :
                                                 (cp[0] == cMax[0] && bmax[0] == Chunk::sizeX - 1) ?
                                                 blockSide_t::PX : blockSide_t::NONE )) |
                                               (((cp[1] == cMin[1] && bmin[1] == 0) ? blockSide_t::NY :
                                                 (cp[1] == cMax[1] && bmax[1] == Chunk::sizeY - 1) ?
                                                 blockSide_t::PY : blockSide_t::NONE )) |
                                               (((cp[2] == cMin[2] && bmin[2] == 0) ? blockSide_t::NZ :
                                                 (cp[2] == cMax[2] && bmax[2] == Chunk::sizeZ - 1) ?
                                                 blockSide_t::PZ : blockSide_t::NONE ))));
                }
            }
        }
  for(auto iter : chunkUpdates)
    {
      ChunkPtr chunk = iter.first;
      chunk->updateConnected();
      chunk->setNeedSave(true);
      mChunkMap.updateAdjacent(chunk->pos(), iter.second);
    }
  return true;
}
  
bool World::setRange(const Point3i &p1, const Point3i &p2, block_t type, BlockData *data)
{
  Point3i pp1{std::min(p1[0], p2[0]), std::min(p1[1], p2[1]), std::min(p1[2], p2[2])};
  Point3i pp2{std::max(p1[0], p2[0]), std::max(p1[1], p2[1]), std::max(p1[2], p2[2])};
  Point3i cMin = chunkPos(pp1);
  Point3i cMax = chunkPos(pp2);
  Point3i minB = Chunk::blockPos(pp1);
  Point3i maxB = Chunk::blockPos(pp2);
  std::unordered_map<ChunkPtr, blockSide_t> chunkUpdates;
  Point3i cp;
  for(cp[0] = cMin[0]; cp[0] <= cMax[0]; cp[0]++)
    for(cp[1] = cMin[1]; cp[1] <= cMax[1]; cp[1]++)
      for(cp[2] = cMin[2]; cp[2] <= cMax[2]; cp[2]++)
        {
          ChunkPtr chunk = mChunkMap[cp];
          if(chunk)
            {
              Point3i wp = cp*Chunk::size;
              Point3i bmin{cp[0] == cMin[0] ? minB[0] : 0,
                           cp[1] == cMin[1] ? minB[1] : 0,
                           cp[2] == cMin[2] ? minB[2] : 0};
              Point3i bmax{cp[0] == cMax[0] ? maxB[0] : Chunk::sizeX-1,
                           cp[1] == cMax[1] ? maxB[1] : Chunk::sizeY-1,
                           cp[2] == cMax[2] ? maxB[2] : Chunk::sizeZ-1};
              Point3i bp;
              bool changed = false;
              for(bp[0] = bmin[0]; bp[0] <= bmax[0]; bp[0]++)
                for(bp[1] = bmin[1]; bp[1] <= bmax[1]; bp[1]++)
                  for(bp[2] = bmin[2]; bp[2] <= bmax[2]; bp[2]++)
                    {
                      if((type == block_t::NONE || isSimpleBlock(type)) && chunk->setBlock(bp, type))
                        {
                          changed = true;
                          mFluids.set(wp + bp, nullptr);
                        }
                      else if(isFluidBlock(type) && data &&
                              mFluids.set(wp + bp, reinterpret_cast<Fluid*>(data)) )
                        {
                          changed = true;
                        }
                    }
              if(changed)
                {
                  // if(cp[0] < 0)
                  //   {
                  //     bmin[0] = Chunk::sizeX - bmin[0];
                  //     bmax[0] = Chunk::sizeX - bmax[0];
                  //   }
                  // if(cp[1] < 0)
                  //   {
                  //     bmin[1] = Chunk::sizeY - bmin[1];
                  //     bmax[1] = Chunk::sizeY - bmax[1];
                  //   }
                  // if(cp[2] < 0)
                  //   {
                  //     bmin[2] = Chunk::sizeZ - bmin[2];
                  //     bmax[2] = Chunk::sizeZ - bmax[2];
                  //   }
                  chunkUpdates.emplace(chunk, (((cp[0] == cMin[0] && bmin[0] == 0 ? blockSide_t::NX :
                                                 (cp[0] == cMax[0] && bmax[0] == Chunk::sizeX - 1) ?
                                                 blockSide_t::PX : blockSide_t::NONE )) |
                                               (((cp[1] == cMin[1] && bmin[1] == 0) ? blockSide_t::NY :
                                                 (cp[1] == cMax[1] && bmax[1] == Chunk::sizeY - 1) ?
                                                 blockSide_t::PY : blockSide_t::NONE )) |
                                               (((cp[2] == cMin[2] && bmin[2] == 0) ? blockSide_t::NZ :
                                                 (cp[2] == cMax[2] && bmax[2] == Chunk::sizeZ - 1) ?
                                                 blockSide_t::PZ : blockSide_t::NONE ))));
                }
            }
        }
  for(auto iter : chunkUpdates)
    {
      ChunkPtr chunk = iter.first;
      chunk->updateConnected();
      chunk->setNeedSave(true);
      mChunkMap.updateAdjacent(chunk->pos(), iter.second);
    }
  return true;
}

bool World::setRange(const Point3i &center, int rad, block_t type, BlockData *data)
{
  Point3i minP = chunkPos(center - rad);
  Point3i maxP = chunkPos(center + rad);
  if(minP == maxP)
    {
      ChunkPtr chunk = mChunkMap[minP];
      if(chunk)
        {
          Point3i wp = minP*Chunk::size;
          Point3i minB = Chunk::blockPos(center - rad);
          Point3i maxB = Chunk::blockPos(center + rad);
          Point3i bp;
          blockSide_t edges = blockSide_t::NONE;
          for(bp[0] = minB[0]; bp[0] <= maxB[0]; bp[0]++)
            for(bp[1] = minB[1]; bp[1] <= maxB[1]; bp[1]++)
              for(bp[2] = minB[2]; bp[2] <= maxB[2]; bp[2]++)
                {
                  if((type == block_t::NONE || isSimpleBlock(type)) && chunk->setBlock(bp, type))
                    {
                      mFluids.set(wp + bp, nullptr);
                      edges |= Chunk::chunkEdge(bp);
                    }
                  else if(isFluidBlock(type) && data)
                    {
                      if(mFluids.set(wp + bp, reinterpret_cast<Fluid*>(data)))
                        {
                          edges |= Chunk::chunkEdge(bp);
                        }
                    }
                }
          chunk->updateConnected();
          chunk->setNeedSave(true);
          // chunk->setPriority(true);
          // chunk->setDirty(true);
          mChunkMap.updateAdjacent(minP, edges);
          return true;
        }
    }
  else
    {
      Point3i minB = Chunk::blockPos(center - rad);
      Point3i maxB = Chunk::blockPos(center + rad);
      Point3i cp;
      for(cp[0] = minP[0]; cp[0] <= maxP[0]; cp[0]++)
        for(cp[1] = minP[1]; cp[1] <= maxP[1]; cp[1]++)
          for(cp[2] = minP[2]; cp[2] <= maxP[2]; cp[2]++)
            {
              ChunkPtr chunk = mChunkMap[cp];
              if(chunk)
                {
                  Point3i wp = cp*Chunk::size;
                  Point3i bmin{cp[0] == minP[0] ? minB[0] : 0,
                               cp[1] == minP[1] ? minB[1] : 0,
                               cp[2] == minP[2] ? minB[2] : 0};
                  Point3i bmax{cp[0] == maxP[0] ? maxB[0] : Chunk::sizeX-1,
                               cp[1] == maxP[1] ? maxB[1] : Chunk::sizeY-1,
                               cp[2] == maxP[2] ? maxB[2] : Chunk::sizeZ-1};
                  blockSide_t edges = blockSide_t::NONE;
                  Point3i bp;
                  for(bp[0] = bmin[0]; bp[0] <= bmax[0]; bp[0]++)
                    for(bp[1] = bmin[1]; bp[1] <= bmax[1]; bp[1]++)
                      for(bp[2] = bmin[2]; bp[2] <= bmax[2]; bp[2]++)
                        {
                          if((type == block_t::NONE || isSimpleBlock(type)) && chunk->setBlock(bp, type))
                            {
                              mFluids.set(wp + bp, nullptr);
                              edges |= Chunk::chunkEdge(bp);
                            }
                          else if(isFluidBlock(type) && data)
                            {
                              if(mFluids.set(wp + bp, reinterpret_cast<Fluid*>(data)))
                                {
                                  edges |= Chunk::chunkEdge(bp);
                                }
                            }
                        }
                  chunk->updateConnected();
                  chunk->setNeedSave(true);
                  // chunk->setPriority(true);
                  // chunk->setDirty(true);
                  mChunkMap.updateAdjacent(minP, edges);
                }
            }
      return true;
    }
  return false;
}

block_t World::getBlock(const Point3i &wp)
{
  ChunkPtr chunk = mChunkMap[chunkPos(wp)];
  if(chunk)
    { return chunk->getType(Chunk::blockPos(wp)); }
  else
    { return block_t::NONE; }
}

block_t* World::atBlock(const Point3i &wp)
{
  ChunkPtr chunk = mChunkMap[chunkPos(wp)];
  if(chunk)
    { return chunk->at(Chunk::blockPos(wp)); }
  else
    { return nullptr; }
}

void World::addChunkFace(MeshData &data, Chunk *chunk, const Point3i &cp, bool meshed)
{
  Vector3f color = (meshed ? Vector3f{0,0,0} : Vector3f{1,0,0});
  int nVert = data.vertices().size();
  data.vertices().emplace_back((cp + Point3i{0,0,0})*Chunk::size, color, Point2f{0.3,0});
  data.vertices().emplace_back((cp + Point3i{0,1,0})*Chunk::size, color, Point2f{0.3,0});
  data.vertices().emplace_back((cp + Point3i{1,1,0})*Chunk::size, color, Point2f{0.3,0});
  data.vertices().emplace_back((cp + Point3i{1,0,0})*Chunk::size, color, Point2f{0.3,0});
  
  data.vertices().emplace_back((cp + Point3i{0,0,1})*Chunk::size, color, Point2f{0.3,0});
  data.vertices().emplace_back((cp + Point3i{0,1,1})*Chunk::size, color, Point2f{0.3,0});
  data.vertices().emplace_back((cp + Point3i{1,1,1})*Chunk::size, color, Point2f{0.3,0});
  data.vertices().emplace_back((cp + Point3i{1,0,1})*Chunk::size, color, Point2f{0.3,0});

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

  // add render path
  
  nVert = data.vertices().size();
  int i = 0;
  //TreeNode tree = mChunkMap.getTree();//mRenderer->getRenderOrder();
  std::vector<hash_t> tree = mChunkMap.getVisible(mCamera);

  hash_t lastHash = 0;
  for(auto hash : tree)
    {
      float a = (float)i/(float)tree.size();
      Vector3f color1{1,0,0};
      Vector3f color2{1,1,0};
      Vector3f color3{0,1,0};
      Vector3f color4{0,1,1};
      Vector3f lineCol = (a < 0.33 ? lerp(color1, color2, a/0.33) :
                          (a < 0.66 ? lerp(color2, color3, (a-0.33)/0.33) :
                           lerp(color3, color4, (a-0.66)/0.33) ));
      Point2f lineTex{1.0, 0};
      
      data.vertices().emplace_back((Hash::unhash(lastHash) - mMinChunk)*Chunk::size + Chunk::size/2,
                                   lineCol, lineTex);
      data.vertices().emplace_back((Hash::unhash(hash) - mMinChunk)*Chunk::size + Chunk::size/2,
                                   lineCol, lineTex);
      data.indices().emplace_back(nVert + (i++));
      data.indices().emplace_back(nVert + (i++));
      lastHash = hash;
    }

  /*
    int maxSteps = tree.maxDepth();

  std::queue<TreeNode*> nodeQueue;
  nodeQueue.push(&tree);
  
  while(nodeQueue.size() > 0)
    {
      TreeNode *node = nodeQueue.front();
      nodeQueue.pop();

      if(!node->parent)
        { continue; }
      
      float a = (float)node->depth/(float)maxSteps;
      Vector3f color1{1,0,0};
      Vector3f color2{1,1,0};
      Vector3f color3{0,1,0};
      Vector3f color4{0,1,1};
      Vector3f lineCol = (a < 0.33 ? lerp(color1, color2, a/0.33) :
                          (a < 0.66 ? lerp(color2, color3, (a-0.33)/0.33) :
                           lerp(color3, color4, (a-0.66)/0.33) ));
      Point2f lineTex{1.0, 0};
      if(!node->visible)
        {
          lineCol = Vector3f{0.9,0.9,0.9};
          //lineTex[0] = 0.1;
        }
      //      LOGD("HASH 1: 0x%006X | 2: 0x%0006X", line.first, line.second);
          std::cout << "POINT: " << node->parent->pos << " : " << node->pos << "\n";
      if(pointInRange(node->parent->pos, mMinChunk, mMaxChunk) &&
         pointInRange(node->pos, mMinChunk, mMaxChunk) )
        {
          data.vertices().emplace_back((node->parent->pos - mMinChunk)*Chunk::size + Chunk::size/2,
                                       lineCol, lineTex);
          data.vertices().emplace_back((node->pos - mMinChunk)*Chunk::size + Chunk::size/2,
                                       lineCol, lineTex);
          data.indices().emplace_back(nVert + (i++));
          data.indices().emplace_back(nVert + (i++));
        }
    }
  */
  
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

          ChunkPtr chunk = mChunkMap[mMinChunk+cp];
          if(chunk)
            //{ addChunkFace(data, chunk.get(), cp, meshed); }
            { addChunkFace(data, chunk, cp, meshed); }
        }
  
  return data;
}

void World::chunkLoadCallback(Chunk *chunk)
{
  mChunkMap.chunkFinishedLoading(chunk);
}


void World::setCenter(const Point3i &chunkCenter)
{
  //std::lock_guard<std::mutex> lock(mChunkLock);
  LOGI("Moving center from (%d, %d, %d) to (%d, %d, %d)", mCenter[0], mCenter[1], mCenter[2],
       chunkCenter[0], chunkCenter[1], chunkCenter[2] );
  mCenter = chunkCenter;
  mMinChunk = mCenter - mLoadRadius;
  mMaxChunk = mCenter + mLoadRadius;
  mFluids.setRange(mMinChunk, mMaxChunk);
  
  auto unload = mChunkMap.unloadOutside(mMinChunk-1, mMaxChunk+1);
  for(auto hash : unload)
    {
      mRenderer->unload(hash);
      //mRayTracer->unload(hash);
      mVisualizer->unload(hash);
    }

  mRenderer->setCenter(mCenter);
  //mRayTracer->setCenter(mCenter);
  mVisualizer->setCenter(mCenter);
}

void World::setRadius(const Vector3i &chunkRadius)
{
  //std::lock_guard<std::mutex> lock(mChunkLock);
  LOGI("Setting chunk radius to (%d, %d, %d)", chunkRadius[0], chunkRadius[1], chunkRadius[2]);
  mLoadRadius = chunkRadius;
  mChunkDim = mLoadRadius * 2 + 1;
  mMinChunk = mCenter - mLoadRadius;
  mMaxChunk = mCenter + mLoadRadius;
  
  mFluids.setRange(mMinChunk, mMaxChunk);
  auto unload = mChunkMap.unloadOutside(mMinChunk, mMaxChunk);
  for(auto hash : unload)
    {
      mRenderer->unload(hash);
      //mRayTracer->unload(hash);
      mVisualizer->unload(hash);
    }

  Vector3f bSize = mLoadRadius*Chunk::size;
  if(bSize[0] == 0) bSize[0] += Chunk::sizeX/2;
  if(bSize[1] == 0) bSize[1] += Chunk::sizeY/2;
  if(bSize[2] == 0) bSize[2] += Chunk::sizeZ/2;
  float minDim = std::min(bSize[0], std::min(bSize[1], bSize[2]));
  mDirScale = Vector3f{minDim/bSize[0],minDim/bSize[1],minDim/bSize[2]};
  mDirScale *= mDirScale;
  mFogStart = (minDim)*FOG_START;
  mFogEnd = (minDim)*FOG_END;
  mRadChanged = true;
  mRenderer->setFog(mFogStart, mFogEnd, mDirScale);
  //mRayTracer->setFog(mFogStart, mFogEnd, mDirScale);

  mChunkMap.setRadius(mLoadRadius);
  mRenderer->setRadius(mLoadRadius);
  mVisualizer->setRadius(mLoadRadius);
  
  //mCenterDistIndex = 0;
}
Point3i World::getCenter() const
{ return mCenter; }

void World::setCamera(Camera *camera)
{
  mRenderer->setCamera(camera);
  //mRayTracer->setCamera(camera);
  mChunkMap.setCamera(camera);
  mCamera = camera;
}
void World::setFrustumCulling(bool on)
{
  mRenderer->setFrustumCulling(on);
  //mRayTracer->setFrustumCulling(on);
}
void World::pauseFrustumCulling()
{
  mFrustumPaused = !mFrustumPaused;
  mRenderer->pauseFrustumCulling();
  //mRayTracer->pauseFrustum();
}

void World::setScreenSize(const Point2i &size)
{
  //mRayTracer->setScreenSize(size);
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
