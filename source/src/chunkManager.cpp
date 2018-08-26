#include "chunkManager.hpp"
#include "shader.hpp"
#include "textureAtlas.hpp"
#include "terrain.hpp"

#include <unistd.h>
#include <chrono>

#define LOAD_SLEEP_US 2000

inline int cChunkManager::getThread(const Point3i &cp)
{
  return std::abs((cp[0] + cp[1] + cp[2]) % mUpdatePool.numThreads());
}

// class definitions
cChunkManager::cChunkManager(int loadThreads, const Point3i &center,
			     const Vector3i &loadRadius )
  : mUpdatePool(loadThreads, std::bind(&cChunkManager::updateWorker, this,
                                       std::placeholders::_1 ), LOAD_SLEEP_US ),
    mLoader(1, nullptr),
    mCenter(center), mLoadRadius(loadRadius), mChunkDim(loadRadius * 2 + 1),
    mChunks(loadRadius * 2 + 1, center - loadRadius),
    mMinChunk(center - loadRadius), mMaxChunk(center + loadRadius)
{
  
}

cChunkManager::~cChunkManager()
{
  stop();
}

bool cChunkManager::setWorld(std::string worldName, uint32_t seed)
{
  LOGD("CHUNK MANAGER SETTING WORLD");
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
          exit(1);
        }
    }
  if(!mLoader.loadWorld(worldName))
    {
      LOGD("FAILED TO LOAD WORLD");
      exit(1);
    }

  return true;
}

void cChunkManager::start()
{
  //LOGD("CHUNK MANAGER STARTING");
  //mLoader.start();
  //mUpdating = true;
  mUpdatePool.start();
}
void cChunkManager::stop()
{
  //LOGD("CHUNK MANAGER STOPPING LOADER");
  //mLoader.stop();
  mUpdatePool.stop();
}

int cChunkManager::getHeightAt(const Point3i &hPos)
{
  //std::lock_guard<std::mutex> lock(mChunkLock);
  Point3i cPos{hPos[0] / cChunk::sizeX, hPos[1] / cChunk::sizeY, hPos[2] / cChunk::sizeZ};
  Point3i bPos{hPos[0] % cChunk::sizeX, hPos[1] % cChunk::sizeY, hPos[2] % cChunk::sizeZ};
  //const int chunkIndex = index(getArrayPos(cPos - mMinChunk));
  /*
    cChunk *chunk = mChunks[chunkIndex].chunk;
    while(!chunk)
    {
    usleep(1000);
    chunk = mChunks[chunkIndex].chunk;
    }
    for(int cz = cChunk::sizeZ; cz >= 0; cz--)
    {
    if(chunk->get(bPos[0], bPos[1], cz) != block_t::NONE)
    {
    return chunk->pos()[2] * cChunk::sizeZ + cz + 1;
    }
    }
    return chunk->pos()[2] * cChunk::sizeZ + 1;
  */
  return cPos[2] * cChunk::sizeZ + 2;
}

void cChunkManager::cleanupGL()
{
  delete mBlockShader;
  mTexAtlas->destroy();
  delete mTexAtlas;
  
  std::lock_guard<std::mutex> lock(mChunkLock);
  
  for(int x = 0; x < mChunkDim[0]; x++)
    for(int y = 0; y < mChunkDim[1]; y++)
      for(int z = 0; z < mChunkDim[2]; z++)
        {
          mChunks.staticAccess(x,y,z)->cleanupGL();
        }
}

void cChunkManager::initGL(QObject *qParent)
{
  // load shader
  mBlockShader = new cShader(qParent);
  if(!mBlockShader->loadProgram("./shaders/simpleBlock.vsh", "./shaders/simpleBlock.fsh",
                                {"posAttr", "normalAttr", "texCoordAttr"},
                                {"pvm", "camPos", "fogStart", "fogEnd", "uTex"} ))
    {
      LOGE("Simple block shader failed to load!");
    }
  
  mBlockShader->bind();
  mBlockShader->setUniform("uTex", 0);
  mBlockShader->setUniform("fogStart", 5*(float)mLoadRadius[0]*cChunk::sizeX*0.75f);
  mBlockShader->setUniform("fogEnd", 5*(float)mLoadRadius[0]*cChunk::sizeX*0.8f);
  mBlockShader->release();

  // load block textures
  mTexAtlas = new cTextureAtlas(qParent, ATLAS_BLOCK_SIZE);
  if(!mTexAtlas->create("./res/texAtlas.png"))
    {
      LOGE("Failed to load texture atlas!");
    }
  
  std::lock_guard<std::mutex> lock(mChunkLock);
  for(int x = 0; x < mChunkDim[0]; x++)
    for(int y = 0; y < mChunkDim[1]; y++)
      for(int z = 0; z < mChunkDim[2]; z++)
        {
          mChunks.staticAccess(x,y,z)->initGL(mBlockShader);
        }
}
void cChunkManager::render(const Matrix4 &pvm)
{
  // render all available chunks
  mBlockShader->bind();
  mTexAtlas->bind();
  mBlockShader->setUniform("pvm", pvm);
  mBlockShader->setUniform("camPos", mCamPos);
  
  {
    std::lock_guard<std::mutex> lock(mChunkLock);
    for(int x = 0; x < mChunkDim[0]; x++)
      for(int y = 0; y < mChunkDim[1]; y++)
        for(int z = 0; z < mChunkDim[2]; z++)
          {
            cChunk *chunk = mChunks.staticAccess(x, y, z);
            if(chunk->isLoaded())
              {
                if(!chunk->meshUploaded() && !chunk->meshDirty())
                  {
                    chunk->uploadMesh();
                    chunk->setMeshUploaded(true);
                  }
              }
            chunk->render();
          }
  }
  mBlockShader->release();
  mTexAtlas->release();
}
/*
  void cChunkManager::clearMesh(int chunkIndex)
  {
  //mChunks[chunkIndex]->mesh.getVertices().clear();
  //mChunks[chunkIndex]->mesh.getIndices().clear();
  }
*/
// void cChunkManager::updateMesh(int chunkIndex)
// {
//   ChunkData &chunk = mChunks[chunkIndex];
//   if(chunk.chunk)
//     {
//       updateMesh(chunk);
//     }
// }

// void cChunkManager::requestRange(const Point3i &min, const Point3i &max)
// {
  
// }


#define THREAD_LOOP_MAX_LOAD 8

void cChunkManager::updateWorker(int tid)
{ // update dirty meshes
  std::vector<cChunk*> loadChunks;
  std::vector<cChunk*> readyChunks;
  std::vector<cChunk*> lightChunks;
  std::vector<cChunk*> meshChunks;
  std::vector<cChunk*> saveChunks;
  //{

  {
    std::lock_guard<std::mutex> lock(mChunkLock);
    const Point3i num = mChunkDim / mUpdatePool.numThreads();
    const Point3i start = num * tid;
    const Point3i end = start + num;
    for(int x = start[0]; x < end[0]; x++)
      for(int y = start[1]; y < end[1]; y++)
        for(int z = start[2]; z < end[2]; z++)
          {
            cChunk *chunk = mChunks.staticAccess(x, y, z);
            if(!chunk->isLoaded())
              {
                if(loadChunks.size() >= THREAD_LOOP_MAX_LOAD)
                  { continue; }
                loadChunks.push_back(chunk);
              }
            else if(chunk->meshDirty())
              { meshChunks.push_back(chunk); }
            else if(chunk->isDirty())
              { saveChunks.push_back(chunk); }
          }
  }

  //if(mLightInit && !chunk->isLight())
                //{ lightChunks.push_back(chunk); }


    /*
    std::lock_guard<std::mutex> lock(mChunkLock);

    while(mDeadChunks[tid].size() > 0)
      {
        int cHash = mDeadChunks[tid].front();
        mDeadChunks[tid].pop();
        //mInactiveChunks.push_back(mChunks[cHash]);
        mNewChunks[tid].push(mChunks[cHash]);
        mChunks.erase(cHash);
      }

    for(auto &cPos : mThreadLoop[tid])
      {
            // find if chunk is loaded
            cChunk *chunk = nullptr;
            const int cHash = hashChunk(cPos);
            auto cIter = mChunks.find(cHash);
            if(cIter == mChunks.end())
              { // load chunk, up to maximum per thread loop
                if(loadChunks.size() >= THREAD_LOOP_MAX_LOAD)
                { continue; }

                
                // // take chunk from inactive
                // if(mInactiveChunks.size() > 0)
                //   {
                //     chunk = mInactiveChunks.front();
                //     mInactiveChunks.pop_front();
                //     chunk->unsetNeighbors();
                //     chunk->setWorldPos(cPos);
                //     loadChunks.push_back(chunk);
                //   }
                
                chunk = mNewChunks[tid].front();
                mNewChunks[tid].pop();
                chunk->setWorldPos(cPos);
                loadChunks.push_back(chunk);
              }
            else
              { // update chunk if possible
                chunk = cIter->second;
                //if(chunk->neighborsLoaded())
                if(cPos
                {
                    //if(mLightInit && !chunk->isLight())
                    //{ lightChunks.push_back(chunk); }
                    if(chunk->meshDirty())
                      { meshChunks.push_back(chunk); }
                    else if(chunk->isDirty())
                      { saveChunks.push_back(chunk); }
                  }
                else if(chunk->checkNeighbors())
                  {
                    readyChunks.push_back(chunk);
                  }
              }
          }
  }
    */
  
  for(auto chunk : loadChunks)
    {
      mLoader.loadDirect(chunk);
      mNumLoaded++;
      chunk->setMeshDirty(true);
      chunk->setLoaded(true);
      // std::lock_guard<std::mutex> lock(mChunkLock);
      // for(int i = 0; i < gBlockSides.size(); i++)
      //   { // connect with any existing  neighbors
      //     const blockSide_t side = gBlockSides[i];
      //     const Point3i dir = sideDirection(side);
      //     const int c2Hash = hashChunk(chunk->pos() + dir);
          
      //     auto c2Iter = mChunks.find(c2Hash);
      //     if(c2Iter != mChunks.end())
      //       { chunk->setNeighbor(side, c2Iter->second); }
      //   }
      
      // mChunks.emplace(hashChunk(chunk->pos()), chunk);

    }
  // for(auto chunk : readyChunks)
  //   {
  //     chunk->updateBlocks();
  //     /*
  //     if(chunk->pos()[2] == mMaxChunk[2]-1)
  //       {
  //         LOGD("UPDATING LIGHT! --> (%d, %d, %d)", chunk->pos()[0],chunk->pos()[1],chunk->pos()[2]);
  //         chunk->updateLighting(mLighting);
  //       }
  //     */
  //   }

  //for(auto chunk : lightChunks)
  //{
      //LOGD("UPDATING CHUNK LIGHTING");
      //chunk->updateLighting(mLighting);
      //  }
  for(auto chunk : meshChunks)
    {
      chunk->updateBlocks();
      chunk->updateMesh();
      chunk->setMeshDirty(false);
    }
  for(auto chunk : saveChunks)
    {
      mLoader.saveDirect(chunk);
      chunk->setDirty(false);
    }
}

Point3i cChunkManager::minChunk() const
{ return mMinChunk; }
Point3i cChunkManager::maxChunk() const
{ return mMaxChunk; }

void cChunkManager::setRadius(const Vector3i &loadRadius)
{
  mLoadRadius = loadRadius;
  mChunkDim = loadRadius * 2 + 1;
  mMinChunk = mCenter - mLoadRadius;
  mMaxChunk = mCenter + mLoadRadius;
}

// templated sign function
template <typename T> int sgn(T val) {
  return (T(0) < val) - (val < T(0));
}

void cChunkManager::setCenter(const Point3i &chunkCenter)
{
    std::lock_guard<std::mutex> lock(mChunkLock);
    Point3i diff = chunkCenter - mCenter;
    
    mCenter = chunkCenter;
    mMinChunk = mCenter - mLoadRadius;
    mMaxChunk = mCenter + mLoadRadius;
    mChunks.rotate(diff);

    
    /*
    for(auto &cIter : mChunks)
      {
        cChunk *chunk = cIter.second;
        Point3i cPos = chunk->pos();
        if(cPos[0] < mMinChunk[0] || cPos[0] > mMaxChunk[0] ||
           cPos[1] < mMinChunk[1] || cPos[1] > mMaxChunk[1] ||
           cPos[2] < mMinChunk[2] || cPos[2] > mMaxChunk[2] )
          {
            cIter.second->setLoaded(false);
            Point3i cPos = cIter.second->pos();
            int tid = std::abs((cPos[0]+cPos[1]+cPos[2]) % mUpdatePool.numThreads());
            mDeadChunks[tid].push(cIter.first);
          }
      }
  }
    */
}

Point3i cChunkManager::chunkPoint(const Point3i &worldPos) const
{ return chunkPos(worldPos); }


Point3i cChunkManager::getCenter() const
{
  return mCenter;
}
Vector3i cChunkManager::getRadius() const
{
  return mLoadRadius;
}

void chunkLoadCallback(chunkPtr_t chunk)
{
  /*
    int cHash = hashChunk(chunk->pos());
    {
    std::lock_guard<std::mutex> lock(mChunkLock);
    mChunks[cHash] = chunk;
    }
  */
}


int cChunkManager::numLoaded() const
{
  return mNumLoaded;
}

block_t cChunkManager::get(const Point3i &wp)
{
  
  std::lock_guard<std::mutex> lock(mChunkLock);
  //const int cHash = hashChunk(chunkPos(wp));
  //auto cIter = mChunks.find(cHash);
  Point3i cPos = chunkPos(wp);
  //if(cIter != mChunks.end() && cIter->second->isLoaded())
  if(cPos[0] >= mMinChunk[0] && cPos[0] <= mMaxChunk[0] &&
     cPos[1] >= mMinChunk[1] && cPos[1] <= mMaxChunk[1] &&
     cPos[2] >= mMinChunk[2] && cPos[2] <= mMaxChunk[2] )
    { return mChunks[cPos-mMinChunk]->get(cChunkData::blockPos(wp)); }
  else
    { return block_t::NONE; }
  
  return block_t::NONE;
}
cBlock* cChunkManager::at(const Point3i &wp)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  // const int cHash = hashChunk(chunkPos(wp));
  // auto cIter = mChunks.find(cHash);
  // if(cIter != mChunks.end() && cIter->second->isLoaded())
  Point3i cPos = chunkPos(wp);
  if(cPos[0] >= mMinChunk[0] && cPos[0] <= mMaxChunk[0] &&
     cPos[1] >= mMinChunk[1] && cPos[1] <= mMaxChunk[1] &&
     cPos[2] >= mMinChunk[2] && cPos[2] <= mMaxChunk[2] )
    { return mChunks[cPos-mMinChunk]->at(cChunkData::blockPos(wp)); }
  else
    { return nullptr; }
}
void cChunkManager::set(const Point3i &wp, block_t type)
{
  
  std::lock_guard<std::mutex> lock(mChunkLock);
  // const int cHash = hashChunk(chunkPos(wp));
  // auto cIter = mChunks.find(cHash);
  // if(cIter != mChunks.end() && cIter->second->isLoaded())
  Point3i cPos = chunkPos(wp);
  if(cPos[0] >= mMinChunk[0] && cPos[0] <= mMaxChunk[0] &&
     cPos[1] >= mMinChunk[1] && cPos[1] <= mMaxChunk[1] &&
     cPos[2] >= mMinChunk[2] && cPos[2] <= mMaxChunk[2] )
    { mChunks[cPos-mMinChunk]->set(cChunkData::blockPos(wp), type); }
  
}
void cChunkManager::clear()
{
  // std::cout << "CHUNK MANAGER CLEARING\n";
  // std::lock_guard<std::mutex> lock(mChunkLock);
  // for(auto &c : mChunks)
  //   {
  //     //{
  //     //std::unique_lock<std::mutex> lock(mChunkLock);
  //     //mChunkCv.wait(lock, [&c](){ return !c->processing.exchange(true); });
      
  //     if(c->active && c->chunk->dirty())
  //       {                
  //         if(!mLoader.save(c->chunk))
  //           { LOGE("Error saving chunk! (skipping)"); }
  //         c->chunk->setClean();
  //       }
  //     //}
  //     //c->processing.store(false);
  //     //mChunkCv.notify_one();
  //   }
  // mChunks.clear();
}
void cChunkManager::saveChunks()
{
  // std::cout << "CHUNK MANAGER SAVING\n";
  // for(auto &c : mChunks)
  //   {
  //     //std::unique_lock<std::mutex> lock(mChunkLock);
  //     //mChunkCv.wait(lock, [&c](){ return !c->processing.exchange(true); });
      
  //     if(c->active && c->chunk->dirty())
  //       {
  //         if(!mLoader.save(c->chunk))
  //           { LOGE("Error saving chunk! (skipping)"); }
  //         c->chunk->setClean();
  //       }
  //   }
}

void cChunkManager::setLighting(int lightLevel)
{
  /*
  std::lock_guard<std::mutex> lock(mChunkLock);
  mLighting = lightLevel;
  mLightInit = false;
  // for(auto &cIter : mChunks)
  for(int x = 0; x < mChunkDim[0]; x++)
    for(int y = 0; y < mChunkDim[1]; y++)
      for(int z = 0; z < mChunkDim[2]; z++)
        { mChunks.staticAccess(x,y,z)->setLight(false); }
          //    { cIter.second->setLight(false); }
          */
}

void cChunkManager::updateLighting()
{
  /*
  std::lock_guard<std::mutex> lock(mChunkLock);
  for(int x = 0; x <= mChunkDim[0]; x++)
    for(int y = 0; y <= mChunkDim[1]; y++)
      {
        if(mChunks[Point3i{x,y,mChunkDim[2]-1}]->isLoaded())
          { mChunks[Point3i{x,y,mChunkDim[2]-1}]->updateLighting(mLighting); }
          
        // const int cHash = hashChunk(x, y, mChunkDim[2]-1);
        
        // auto cIter = mChunks.find(cHash);
        // if(cIter != mChunks.end() && cIter->second->isLoaded())
        //   {
        //     cIter->second->updateLighting(mLighting);
        //   }
      }
  mLightInit = true;
*/
}

void cChunkManager::update()
{
  // if(!mLightInit && mNumLoaded == mChunkDim[0]*mChunkDim[1]*mChunkDim[2])
  //   {
  //     updateLighting();
  //     mLightInit = true;
  //   }
}


  
inline int cChunkManager::expand(int x)
{
  x                  &= 0x000003FF;
  x  = (x | (x<<16)) &  0xFF0000FF;
  x  = (x | (x<<8))  &  0x0F00F00F;
  x  = (x | (x<<4))  &  0xC30C30C3;
  x  = (x | (x<<2))  &  0x49249249;
  return x;
}
inline int cChunkManager::unexpand(int x) const
{
  x                 &= 0x49249249;
  x = (x | (x>>2))  &  0xC30C30C3;
  x = (x | (x>>4))  &  0x0F00F00F;
  x = (x | (x>>8))  &  0xFF0000FF;
  x = (x | (x>>16)) &  0x000003FF;
  return (x << 22) >> 22;
}
inline int cChunkManager::hashChunk(int cx, int cy, int cz)
{ return expand(cx) + (expand(cy) << 1) + (expand(cz) << 2); }
inline int cChunkManager::hashChunk(const Point3i &cp)
{ return hashChunk(cp[0], cp[1], cp[2]); }
inline int cChunkManager::unhashX(int cx) const
{ return unexpand(cx); }
inline int cChunkManager::unhashY(int cy) const
{ return unexpand(cy); }
inline int cChunkManager::unhashZ(int cz) const
{ return unexpand(cz); }
//inline Point3i cChunkManager::unhash(const Point3i &rp) const
//{ return Point3i{unexpand(rp[0]), unexpand(rp[1]), unexpand(rp[2])}; }



std::ostream& operator<<(std::ostream &os, const cChunkManager &cm)
{
  os << "<CHUNK MANAGER: " << cm.mChunkDim << " chunks loaded>";
  return os;
}

int cChunkManager::chunkX(int wx)
{ return wx >> cChunkData::shiftX; }
int cChunkManager::chunkY(int wy)
{ return wy >> cChunkData::shiftY; }
int cChunkManager::chunkZ(int wz)
{ return wz >> cChunkData::shiftZ; }
Point3i cChunkManager::chunkPos(const Point3i &wp)
{ return Point3i({chunkX(wp[0]), chunkY(wp[1]), chunkZ(wp[2])}); }

/*
// (OLD) optimized functions for indexing
inline int cChunkManager::index(int cx, int cy, int cz, int sx, int sz)
{ return cx + sx * (cz + sz * cy); }
inline int cChunkManager::index(int cx, int cy, int cz) const
{ return cx + mChunkDim[0] * (cz + mChunkDim[2] * cy); }
inline int cChunkManager::index(const Point3i &cp) const
{ return index(cp[0], cp[1], cp[2]); }

inline Point3i cChunkManager::unflattenIndex(int index) const
{
const int yMult = mChunkDim[0] * mChunkDim[2];
const int yi = index / yMult;
index -= yi * yMult;
const int zi = index / mChunkDim[0];
const int xi = index - zi * mChunkDim[0];
return Point3i({xi, yi, zi});
}

inline int cChunkManager::chunkX(int wx) const
{ return wx >> cChunk::shiftX; }
inline int cChunkManager::chunkY(int wy) const
{ return wy >> cChunk::shiftY; }
inline int cChunkManager::chunkZ(int wz) const
{ return wz >> cChunk::shiftZ; }
inline Point3i cChunkManager::chunkPos(const Point3i &wp) const
{ return Point3i({chunkX(wp[0]), chunkY(wp[1]), chunkZ(wp[2])}); }


inline int cChunkManager::adjPX(int ci) const
{ return ci + 1; }
inline int cChunkManager::adjPY(int ci) const
{ return ci + mChunkDim[0]*mChunkDim[2]; }
inline int cChunkManager::adjPZ(int ci) const
{ return ci + mChunkDim[0]; }

inline int cChunkManager::adjNX(int ci) const
{ return ci - 1; }
inline int cChunkManager::adjNY(int ci) const
{ return ci - mChunkDim[0]*mChunkDim[2]; }
inline int cChunkManager::adjNZ(int ci) const
{ return ci - mChunkDim[0]; }

*/
