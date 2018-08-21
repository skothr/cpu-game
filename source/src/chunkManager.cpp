#include "chunkManager.hpp"
#include "shader.hpp"
#include "textureAtlas.hpp"
#include "terrain.hpp"

#include <unistd.h>
#include <chrono>

#define LOAD_SLEEP_US 2000

inline int cChunkManager::getThread(const Point3i &cp)
{
  return (cp[0] + cp[1] + cp[2]) % mUpdatePool.numThreads();
}

// class definitions
cChunkManager::cChunkManager(int loadThreads, const Point3i &center,
			     const Vector3i &loadRadius )
  : mUpdatePool(loadThreads, std::bind(&cChunkManager::updateWorker, this,
                                       std::placeholders::_1 ), LOAD_SLEEP_US ),
    mLoader(1, nullptr), //mLoadLocks(loadThreads),
    //mLoadQueues(loadThreads), mUnloadQueues(loadThreads),
    mChunks(loadRadius * 2 + 1, center - loadRadius),
    mCenter(center), mLoadRadius(loadRadius), mChunkDim(loadRadius * 2 + 1),
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
        { mChunks[Point3i{x,y,z}]->cleanupGL(); }
}

void cChunkManager::initGL(QObject *qParent)
{
  // load shader
  mBlockShader = new cShader(qParent);
  if(!mBlockShader->loadProgram("./shaders/simpleBlock.vsh", "./shaders/simpleBlock.fsh",
                                {"posAttr", "normalAttr", "texCoordAttr"}, {"pvm", "uTex"} ))
    {
      LOGE("Simple block shader failed to load!");
    }
  
  mBlockShader->bind();
  mBlockShader->setUniform("uTex", 0);
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
          mChunks[Point3i{x,y,z}]->initGL(mBlockShader);
        }
}

void cChunkManager::render(const Matrix4 &pvm)
{
  /*
  { // clean up and delete old chunks
    std::lock_guard<std::mutex> lock(mDeleteChunkLock);
    //LOGD("Deleting old chunks...");
    while(mDeleteChunks.size() > 0)
      {
        int cHash = mDeleteChunks.front();
        mDeleteChunks.pop();
        
        mChunks[cHash]->unsetNeighbors();
        mInactiveChunks.push(mChunks[cHash]);
        mChunks.erase(cHash);
      }
  }

  // gather new chunks
  std::vector<cChunk*> newChunks;
  newChunks.reserve(mNewChunks.size());
  {
    //LOGD("Gathering new chunks...");
    std::lock_guard<std::mutex> lock(mNewChunkLock);
    while(mNewChunks.size() > 0)
      {
        newChunks.push_back(mNewChunks.front());
        mNewChunks.pop();
      }
  }

  // initialize new chunks
  //LOGD("Initializing new chunks...");

  for(auto chunk : newChunks)
    {
      //LOGD("Setting chunk neighbors...");
      Point3i cp = chunk->pos();
      int cHash = hashChunk(chunk->pos());

      for(int i = 0; i < 6; i++)
        {
          int c2Hash = hashChunk(chunk->pos() + nv[i]);
          auto iter = mChunks.find(c2Hash);
          if(iter != mChunks.end())
            {
              //chunk->setNeighbor((normal_t)i, iter->second);
              //iter->second->setNeighbor((normal_t)((i+3)%6), chunk);
              //iter->second->setMeshDirty(true);
            }
        }
    }
  
  for(auto chunk : newChunks)
    {
      chunk->initGL(mBlockShader);
      chunk->setMeshDirty(false);
      mChunks.emplace(hashChunk(chunk->pos()), chunk);
    }
  */

  
  //LOGD("Rendering chunks...");
  // render all available chunks
  mBlockShader->bind();
  mTexAtlas->bind();
  mBlockShader->setUniform("pvm", pvm);
  //LOGD("RENDERING %d CHUNKS", renderChunks.size());
  {
    std::lock_guard<std::mutex> lock(mChunkLock);
    for(int x = 0; x < mChunkDim[0]; x++)
      for(int y = 0; y < mChunkDim[1]; y++)
        for(int z = 0; z < mChunkDim[2]; z++)
          {
            cChunk *chunk = mChunks[Point3i{x,y,z}];
            //if(chunk->isLoaded())
              {
                if(!chunk->meshUploaded() && !chunk->meshDirty())
                  {
                    //chunk->updateMesh();
                    chunk->uploadMesh();
                    chunk->setMeshUploaded(true);
                  }
                //else if(chunk->meshDirty())
                //{
                //  chunk->updateMesh();
                //  chunk->uploadMesh();
                //  chunk->setMeshUploaded(true);
                //}
                chunk->render();
              }
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

void cChunkManager::updateWorker(int tid)
{ // update dirty meshes
  
  std::vector<cChunk*> loadChunks;
  std::vector<cChunk*> meshChunks;
  std::vector<cChunk*> saveChunks;
  {
    std::lock_guard<std::mutex> lock(mChunkLock);
    
    for(int x = 0; x < mChunkDim[0]; x++)
      for(int y = 0; y < mChunkDim[1]; y++)
        for(int z = 0; z < mChunkDim[2]; z++)
          {
            if((x + y + z) % mUpdatePool.numThreads() == tid)
              {
                cChunk *chunk = mChunks[Point3i{x,y,z}];
                if(!chunk->isLoaded())
                  { loadChunks.push_back(chunk); }
                else if(chunk->meshDirty())
                  { meshChunks.push_back(chunk); }
                else if(chunk->isDirty())
                  { saveChunks.push_back(chunk); }
              }
          }
  }
  for(auto chunk : loadChunks)
    {
      mLoader.loadDirect(chunk);
      chunk->updateBlocks();
      mNumLoaded++;
      chunk->updateMesh();
      chunk->setLoaded(true);
      chunk->setMeshDirty(false);
    }

  for(auto chunk : meshChunks)
    {
      chunk->updateMesh();
      chunk->setMeshDirty(false);
    }
  
  for(auto chunk : saveChunks)
    {
      mLoader.saveDirect(chunk);
      chunk->setDirty(false);
    }

}

void cChunkManager::update()
{
  /*
  std::vector<chunkPtr_t> unload;
  {
    std::lock_guard<std::mutex> lock(mChunkLock);
    for(auto c : mChunks)
      {
        Point3i cPos = c->pos();
        if(cPos[0] < mMinChunk[0] || cPos[1] > mMaxChunk[0] ||
           cPos[0] < mMinChunk[0] || cPos[1] > mMaxChunk[0] ||
           cPos[0] < mMinChunk[0] || cPos[1] > mMaxChunk[0] )
          {
            int cHash = hashChunk(cPos);
            unload.push_back(mChunks[cHash]);
            mChunks.erase(cHash);
          }
      }
  }

  for(auto c : unload)
    {
      int cHash = hashChunk(cPos);
      {
        std::lock_guard<std::mutex> lock(mMeshLock);
        cMesh *mesh = mMeshes[cHash];
        mMeshes.erase(cHash);
      }
      delete mesh;
      
      if(c->isDirty())
        { mFileLoader.saveDirect(c); }
      delete c;
      }*/
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
  const Vector3i diff = chunkCenter - mCenter;

  LOGD("Rotating chunks --> %d,%d,%d", diff[0], diff[1], diff[2]);
  
  mCenter = chunkCenter;
  mMinChunk = mCenter - mLoadRadius;
  mMaxChunk = mCenter + mLoadRadius;
  mChunks.rotate(diff);
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

//cBlock* cChunkManager::get(const Point3i &wp)
block_t cChunkManager::get(const Point3i &wp)
{
  //std::cout << "CHUNK MANAGER GETTING BLOCK\n";
  std::lock_guard<std::mutex> lock(mChunkLock);
  //int cHash = hashChunk(chunkPos(wp));
  //auto iter = mChunks.find(cHash);
  const Point3i cp = chunkPos(wp);
  if(cp[0] >= mMinChunk[0] && cp[0] <= mMaxChunk[0] &&
     cp[1] >= mMinChunk[1] && cp[1] <= mMaxChunk[1] &&
     cp[2] >= mMinChunk[2] && cp[2] <= mMaxChunk[2] )
    {
      return mChunks[cp - mMinChunk]->get(cChunkData::blockPos(wp));
    }
  else
    {
      return block_t::NONE;
    }
}
void cChunkManager::set(const Point3i &wp, block_t type)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  //int cHash = hashChunk(chunkPos(wp));
  //auto iter = mChunks.find(cHash);
  //if(iter != mChunks.end())//ci >= 0 && ci < mChunks.size() && mChunks[ci]->chunk->isActive())
  const Point3i cp = chunkPos(wp);
  if(cp[0] >= mMinChunk[0] && cp[0] <= mMaxChunk[0] &&
     cp[1] >= mMinChunk[1] && cp[1] <= mMaxChunk[1] &&
     cp[2] >= mMinChunk[2] && cp[2] <= mMaxChunk[2] )
  {
      bool changed = mChunks[cp - mMinChunk]->set(cChunkData::blockPos(wp), type);
      //iter->second->setMeshDirty(true);
      //std::cout << "ADJACENT: " << (int)adj << "\n";
    }
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








inline int cChunkManager::chunkX(int wx) const
{ return wx >> cChunkData::shiftX; }
inline int cChunkManager::chunkY(int wy) const
{ return wy >> cChunkData::shiftY; }
inline int cChunkManager::chunkZ(int wz) const
{ return wz >> cChunkData::shiftZ; }
inline Point3i cChunkManager::chunkPos(const Point3i &wp) const
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
