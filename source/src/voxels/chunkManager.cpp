#include "chunkManager.hpp"
#include "shader.hpp"
#include "textureAtlas.hpp"
#include "terrain.hpp"

#include <unistd.h>
#include <chrono>

#define LOAD_SLEEP_US 2000

inline int cChunkManager::getThread(const Point3i &cp)
{
  return 0;//std::abs((cp[0] + cp[1] + cp[2]) % mUpdatePool.numThreads());
}

// class definitions
cChunkManager::cChunkManager(int loadThreads, const Point3i &center,
			     const Vector3i &loadRadius )
/*
  : mUpdatePool(loadThreads, std::bind(&cChunkManager::updateWorker, this,
                                       std::placeholders::_1 ), LOAD_SLEEP_US ),
    mLoader(1, nullptr),
    mCenter(center), mLoadRadius(loadRadius), mChunkDim(loadRadius * 2 + 1),
    mChunks(loadRadius * 2 + 1, center - loadRadius),
    mMinChunk(center - loadRadius), mMaxChunk(center + loadRadius)
*/
{
  
}

cChunkManager::~cChunkManager()
{
  stop();
}

bool cChunkManager::setWorld(std::string worldName, uint32_t seed)
{
  /*
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
  */
  return true;
}

void cChunkManager::start()
{
  //mUpdatePool.start();
}
void cChunkManager::stop()
{
  //mUpdatePool.stop();
}

int cChunkManager::getHeightAt(const Point3i &hPos)
{
  /*
  //std::lock_guard<std::mutex> lock(mChunkLock);
  Point3i cPos{hPos[0] / cChunk::sizeX, hPos[1] / cChunk::sizeY, hPos[2] / cChunk::sizeZ};
  Point3i bPos{hPos[0] % cChunk::sizeX, hPos[1] % cChunk::sizeY, hPos[2] % cChunk::sizeZ};
  //const int chunkIndex = index(getArrayPos(cPos - mMinChunk));
  
    // cChunk *chunk = mChunks[chunkIndex].chunk;
    // while(!chunk)
    // {
    // usleep(1000);
    // chunk = mChunks[chunkIndex].chunk;
    // }
    // for(int cz = cChunk::sizeZ; cz >= 0; cz--)
    // {
    // if(chunk->get(bPos[0], bPos[1], cz) != block_t::NONE)
    // {
    // return chunk->pos()[2] * cChunk::sizeZ + cz + 1;
    // }
    // }
    // return chunk->pos()[2] * cChunk::sizeZ + 1;
  
  return cPos[2] * cChunk::sizeZ + 2;
  */
  return 0;
}

void cChunkManager::cleanupGL()
{
  /*
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
  */
}

void cChunkManager::initGL(QObject *qParent)
{
  /*
  // load shader
  mBlockShader = new Shader(qParent);
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
  */
}
void cChunkManager::render(const Matrix4 &pvm)
{
  /*
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
  */
}

#define THREAD_LOOP_MAX_LOAD 8
void cChunkManager::updateWorker(int tid)
{ // update dirty meshes

}

Point3i cChunkManager::minChunk() const
{ return Point3i(); }//mMinChunk; }
Point3i cChunkManager::maxChunk() const
{ return Point3i(); }//mMaxChunk; }

void cChunkManager::setRadius(const Vector3i &loadRadius)
{
  // mLoadRadius = loadRadius;
  // mChunkDim = loadRadius * 2 + 1;
  // mMinChunk = mCenter - mLoadRadius;
  // mMaxChunk = mCenter + mLoadRadius;
}

// templated sign function
template <typename T> int sgn(T val) {
  return (T(0) < val) - (val < T(0));
}
void cChunkManager::setCenter(const Point3i &chunkCenter)
{
  /*
    std::lock_guard<std::mutex> lock(mChunkLock);
    Point3i diff = chunkCenter - mCenter;
    
    mCenter = chunkCenter;
    mMinChunk = mCenter - mLoadRadius;
    mMaxChunk = mCenter + mLoadRadius;
    mChunks.rotate(diff);
*/
}

Point3i cChunkManager::chunkPoint(const Point3i &worldPos) const
{ return Point3i(); }//chunkPos(worldPos); }


Point3i cChunkManager::getCenter() const
{
  return Point3i();//mCenter;
}
Vector3i cChunkManager::getRadius() const
{
  return Vector3i();//mLoadRadius;
}

void chunkLoadCallback(Chunk* chunk)
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
  return 0;//mNumLoaded;
}

block_t cChunkManager::get(const Point3i &wp)
{
  /*  
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
  */
  return block_t::NONE;
}
Block* cChunkManager::at(const Point3i &wp)
{
  /*
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
  */
  return nullptr;
}
void cChunkManager::set(const Point3i &wp, block_t type)
{
  /*
  std::lock_guard<std::mutex> lock(mChunkLock);
  // const int cHash = hashChunk(chunkPos(wp));
  // auto cIter = mChunks.find(cHash);
  // if(cIter != mChunks.end() && cIter->second->isLoaded())
  Point3i cPos = chunkPos(wp);
  if(cPos[0] >= mMinChunk[0] && cPos[0] <= mMaxChunk[0] &&
     cPos[1] >= mMinChunk[1] && cPos[1] <= mMaxChunk[1] &&
     cPos[2] >= mMinChunk[2] && cPos[2] <= mMaxChunk[2] )
    { mChunks[cPos-mMinChunk]->set(cChunkData::blockPos(wp), type); }
  */  
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


std::ostream& operator<<(std::ostream &os, const cChunkManager &cm)
{
  //os << "<CHUNK MANAGER: " << cm.mChunkDim << " chunks loaded>";
  return os;
}

int cChunkManager::chunkX(int wx)
{ return 0;}//wx >> cChunkData::shiftX; }
int cChunkManager::chunkY(int wy)
{ return 0;}//wy >> cChunkData::shiftY; }
int cChunkManager::chunkZ(int wz)
{ return 0;}//wz >> cChunkData::shiftZ; }
Point3i cChunkManager::chunkPos(const Point3i &wp)
{ return Point3i({chunkX(wp[0]), chunkY(wp[1]), chunkZ(wp[2])}); }
