#include "chunkManager.hpp"
#include "shader.hpp"

#include <unistd.h>
#include <chrono>

#define LOAD_SLEEP_US 1000

// class definitions
cChunkManager::cChunkManager(int loadThreads, const Point3i &center,
			     const Vector3i &loadRadius )
  : mPool(loadThreads, std::bind(&cChunkManager::updateChunks, this,
                                 std::placeholders::_1 ), LOAD_SLEEP_US ),
    mCenter(center), mLoadRadius(loadRadius), mChunkDim(loadRadius * 2 + 1),
    mMinChunk(center - loadRadius), mMaxChunk(center + loadRadius),
    mNoise(128397304)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  std::cout << "cChunkManager constructing --> " << mCenter << ", " << mLoadRadius << "\n";
  mChunks.reserve(mChunkDim[0] * mChunkDim[1] * mChunkDim[2]*2);

  for(int i = 0; i < (mChunkDim[0]*mChunkDim[1]*mChunkDim[2]); i++)
    {
      const Point3i chunkPos = mMinChunk + unflattenIndex(i);
      mChunks.push_back(new ChunkData(chunkPos));
      if(chunkPos == center - Vector3i{0,0,1})
        {
          mChunks[i]->chunk->setWorldPos(mCenter - Vector3i{0,0,1});
	  //mChunks[i]->loading = true;
          // load chunk below the center first so player doesn't fall while loading
          mLoader.load(mChunks[i]->chunk);
        }
    }

  const int total = (mChunkDim[0]*mChunkDim[1]*mChunkDim[2]);
  for(int i = 0; i < total; i++)
    {
      int next = (adjPX(i)+total) % total;
      //std::cout << "PX! Next: " << next << "\n";
      mChunks[i]->chunk->setNeighbor(normal_t::PX, mChunks[next]->chunk);
      mChunks[next]->chunk->setNeighbor(normal_t::NX, mChunks[i]->chunk);
      
      next = (adjNX(i)+total) % total;
      //std::cout << "NX! Next: " << next << "\n";
      mChunks[i]->chunk->setNeighbor(normal_t::NX, mChunks[next]->chunk);
      mChunks[next]->chunk->setNeighbor(normal_t::PX, mChunks[i]->chunk);
      next = (adjPY(i)+total) % total;
      //std::cout << "PY! Next: " << next << "\n";
      mChunks[i]->chunk->setNeighbor(normal_t::PY, mChunks[next]->chunk);
      mChunks[next]->chunk->setNeighbor(normal_t::NY, mChunks[i]->chunk);
      next = (adjNY(i)+total) % total;
      //std::cout << "NY! Next: " << next << "\n";
      mChunks[i]->chunk->setNeighbor(normal_t::NY, mChunks[next]->chunk);
      mChunks[next]->chunk->setNeighbor(normal_t::PY, mChunks[i]->chunk);
      next = (adjPZ(i)+total) % total;
      //std::cout << "PZ! Next: " << next << "\n";
      mChunks[i]->chunk->setNeighbor(normal_t::PZ, mChunks[next]->chunk);
      mChunks[next]->chunk->setNeighbor(normal_t::NZ, mChunks[i]->chunk);
      next = (adjNZ(i)+total) % total;
      //std::cout << "NZ! Next: " << next << "\n";
      mChunks[i]->chunk->setNeighbor(normal_t::NZ, mChunks[next]->chunk);
      mChunks[next]->chunk->setNeighbor(normal_t::PZ, mChunks[i]->chunk);
      
      /*
      Point3i arrPos = unflattenIndex(i);
      if(arrPos[0] < mChunkDim[0])
        {
          //const int next = index(Vector3i{(arrPos[0] + 1) % mChunkDim[0],arrPos[1],arrPos[2]});
          const int next = adjPX(i) % (mChunkDim[0]*mChunkDim[1]*mChunkDim[2]);
          std::cout << "PX! Next: " << next << "\n";
          mChunks[i]->chunk->setNeighbor(blockSide_t::PX, mChunks[next]->chunk);
          mChunks[next]->chunk->setNeighbor(blockSide_t::NX, mChunks[i]->chunk);
        }
      else if(arrPos[0] > 0)
        {
          //const int next = index(Vector3i{(arrPos[0] - 1) % mChunkDim[0],arrPos[1],arrPos[2]});
          const int next = adjNX(i) % (mChunkDim[0]*mChunkDim[1]*mChunkDim[2]);
          std::cout << "NX!\n";
          mChunks[i]->chunk->setNeighbor(blockSide_t::NX, mChunks[next]->chunk);
          mChunks[next]->chunk->setNeighbor(blockSide_t::PX, mChunks[i]->chunk);
        }
      if(arrPos[1] < mChunkDim[1])
        {
          //const int next = index(Vector3i{arrPos[0], (arrPos[1] + 1) % mChunkDim[1],arrPos[2]});
          const int next = adjPY(i) % (mChunkDim[0]*mChunkDim[1]*mChunkDim[2]);
          std::cout << "PY!\n";
          mChunks[i]->chunk->setNeighbor(blockSide_t::PY, mChunks[next]->chunk);
          mChunks[next]->chunk->setNeighbor(blockSide_t::NY, mChunks[i]->chunk);
        }
      else if(arrPos[1] > 0)
        {
          //const int next = index(Vector3i{arrPos[0], (arrPos[1] + 1) % mChunkDim[1],arrPos[2]});
          const int next = adjNY(i) % (mChunkDim[0]*mChunkDim[1]*mChunkDim[2]);
          std::cout << "NY!\n";
          mChunks[i]->chunk->setNeighbor(blockSide_t::NY, mChunks[next]->chunk);
          mChunks[next]->chunk->setNeighbor(blockSide_t::PY, mChunks[i]->chunk);
        }
              
      if(arrPos[2] < mChunkDim[2])
        {
          //const int next = index(Vector3i{arrPos[0],arrPos[1],(arrPos[2] + 1) % mChunkDim[2]});
          const int next = adjPZ(i) % (mChunkDim[0]*mChunkDim[1]*mChunkDim[2]);
          std::cout << "PZ!\n";
          mChunks[i]->chunk->setNeighbor(blockSide_t::PZ, mChunks[next]->chunk);
          mChunks[next]->chunk->setNeighbor(blockSide_t::NZ, mChunks[i]->chunk);
        }
      else if(arrPos[2] > 0)
        {
          //const int next = index(Vector3i{arrPos[0],arrPos[1],(arrPos[2] + 1) % mChunkDim[2]});
          const int next = adjNZ(i) % (mChunkDim[0]*mChunkDim[1]*mChunkDim[2]);
          std::cout << "NZ!\n";
          mChunks[i]->chunk->setNeighbor(blockSide_t::NZ, mChunks[next]->chunk);
          mChunks[next]->chunk->setNeighbor(blockSide_t::PZ, mChunks[i]->chunk);
        }
      */
    }
  
}

cChunkManager::~cChunkManager()
{
  //mFileManager.stop();
  //clear();
  mPool.stop();
}

void cChunkManager::setSeed(uint32_t seed)
{
  mSeed = seed;
  mNoise.setSeed(seed);
  mLoader.setSeed(seed);
}

bool cChunkManager::setWorld(const std::string &worldName)
{
  bool result =  mLoader.loadWorld(worldName);
  mSeed = mLoader.getSeed();
  return result;
}

void cChunkManager::start()
{
  //mFileManager.start();
  mPool.start();
}
void cChunkManager::stop()
{
  //mFileManager.stop();
  mPool.start();
}

int cChunkManager::getHeightAt(const Point3i &hPos)
{
  
  //std::lock_guard<std::mutex> lock(mChunkLock);
  Point3i cPos{hPos[0] / cChunk::sizeX, hPos[1] / cChunk::sizeY, hPos[2] / cChunk::sizeZ};
  Point3i bPos{hPos[0] % cChunk::sizeX, hPos[1] % cChunk::sizeY, hPos[2] % cChunk::sizeZ};
  const int chunkIndex = index(getArrayPos(cPos - mMinChunk));
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

Point3i cChunkManager::getArrayPos(const Point3i &cp) const
{ // convert position relative to center (starting at mMinChunk) to raw 3d array offset
  return (cp + mCenterShift + mChunkDim) % mChunkDim;
}
Point3i cChunkManager::getRelativePos(const Point3i &ap) const
{ // convert raw 3d array offset to center-relative position
  return (ap - mCenterShift + mChunkDim) % mChunkDim;
}


static std::unordered_map<blockSide_t, std::array<cSimpleVertex, 4>> faceVertices
  {
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


static std::array<unsigned int, 6> faceIndices = { 0, 1, 2, 3, 1, 0 };

void cChunkManager::cleanupGL()
{
  //std::cout << "CHUNK MANAGER CLEANUPGL\n";
  std::lock_guard<std::mutex> lock(mChunkLock);
  for(auto &c : mChunks)
    { c->mesh.cleanupGL(); }
}

void cChunkManager::initGL(cShader *shader)
{
  //std::cout << "CHUNK MANAGER INITGL\n";
  std::lock_guard<std::mutex> lock(mChunkLock);
  for(auto &c : mChunks)
    { c->mesh.initGL(shader); }
}

void cChunkManager::renderSimple(cShader *shader, Matrix4 pvm)
{
  //std::cout << "CHUNK MANAGER RENDERING\n";
  //std::lock_guard<std::mutex> lock(mChunkLock);
  shader->setUniform("pvm", pvm);
  for(int i = 0; i < mChunks.size(); i++)
    {
      ChunkData *c = mChunks[i];
      //if(!c->processing.exchange(true))
      {
        //if(!c->processing.exchange(true))
        //{
            
            if(mChunks[i]->meshDirty)
              { // need mesh update
                mChunks[i]->mesh.detachData();
                updateMesh(mChunks[i], mChunks[i]->chunk->pos());
                mChunks[i]->meshDirty = false;
              }
            
            if(!c->mesh.initialized())
              { c->mesh.initGL(shader); }
            c->mesh.render(shader);
            c->processing.store(false);
            // }
      }
    }
}
/*
  void cChunkManager::renderComplex(cShader *shader, Matrix4 pvm)
  {
  shader->setUniform("pvm", pvm);
  for(int i = 0; i < mComplexMeshes.size(); i++)
  {
  shader->setUniform("uBlockType", (int)complexType(i) - 1);
  mComplexMeshes[i].render(shader);
  }
  }
*/


void cChunkManager::clearMesh(int chunkIndex)
{
  mChunks[chunkIndex]->mesh.getVertices().clear();
  mChunks[chunkIndex]->mesh.getIndices().clear();
}

// void cChunkManager::updateMesh(int chunkIndex)
// {
//   ChunkData &chunk = mChunks[chunkIndex];
//   if(chunk.chunk)
//     {
//       updateMesh(chunk);
//     }
// }

void cChunkManager::updateMesh(ChunkData *chunk, const Point3i &chunkPos)
{
  //Point3i chunkPos = chunk.chunk->pos();
  //std::cout << "Updating mesh for chunk --> " << chunkPos << "\n";

  // recompile mesh
  std::vector<cSimpleVertex> *vertices = &chunk->mesh.getVertices();
  std::vector<unsigned int> *indices = &chunk->mesh.getIndices();
  vertices->clear();
  indices->clear();
  
  const Point3i chunkOffset = (chunkPos *
			       Point3i{cChunk::sizeX, cChunk::sizeY, cChunk::sizeZ} );
  // iterate over the chunk's blocks and compile all face vertices.
  for(int bx = 0; bx < cChunk::sizeX; bx++)
    for(int by = 0; by < cChunk::sizeY; by++)
      for(int bz = 0; bz < cChunk::sizeZ; bz++)
	{
	  const cBlock *block = chunk->chunk->at(bx, by, bz);
	  if(block->active() && isSimpleBlock(block->type))
	    {
	      for(auto &f : faceVertices)
		{ // check if each face direction is active
		  if((bool)(block->activeSides & f.first))
		    {
		      const unsigned int numVert = vertices->size();
		      for(auto v : f.second)
			{
			  vertices->emplace_back(v.pos + chunkOffset + Point3i{bx,by,bz},
						 v.normal,
						 v.texcoord,
						 (int)block->type - 1);
			}
		      for(auto i : faceIndices)
			{ indices->push_back(numVert + i); }
		    }			      
		}
	    }
	  // TODO: Complex blocks
	}
  chunk->mesh.setUpdate();
}

void cChunkManager::update()
{

}
  
void cChunkManager::updateChunks(int id)
{ // update dirty meshes
    
  auto now = std::chrono::steady_clock::now();
  static auto last = now;
  mSaveTimer += std::chrono::duration_cast<std::chrono::microseconds>(now - last).count() / 1000000.0;
  bool save = false;
  if(mSaveTimer >= WRITE_FREQUENCY)
    {
      mSaveTimer = 0.0;
      save = true;
      last = now;
    }

  //std::lock_guard<std::mutex> lock(mChunkLock);
  std::vector<uint8_t> chunkData;

  int threadData = mChunks.size() / mPool.numThreads();
  int threadStart = id * threadData;
  int threadEnd = threadStart + threadData;
  
  for(int i = threadStart; i < threadEnd; i++)
    {
      if(!mChunks[i]->processing.exchange(true))
        { // chunk is now reserved for processing on this thread.
          const Point3i relPos = getRelativePos(unflattenIndex(i));
          Point3i cp = mMinChunk + relPos;
          if(mChunks[i]->active)
            { // chunk is (or was) actively loaded
              if(mChunks[i]->chunk->dirty())
                { // need to save
                  if(save || mChunks[i]->unload)
                    { // save right now
                      if(!mLoader.save(mChunks[i]->chunk))
                        {
                          LOGE("Failed to save chunk!");
                          continue;
                        }
                      mChunks[i]->chunk->setClean();
                    }
                }
              if(mChunks[i]->unload)
                { // unload chunk
                  mChunks[i]->active = false;
                  mNumLoaded--;
                }
              else if(mChunks[i]->meshDirty)
                { // need mesh update
                  //mChunks[i]->mesh.detachData();
                  //updateMesh(mChunks[i], cp);
                  //mChunks[i]->meshDirty = false;
                }
            }
        
          if(!mChunks[i]->active)
          {
            //std::cout << "LOADING CHUNK AT POS: " << cp << "\n";
            mChunks[i]->chunk->setWorldPos(cp);
            if(!mLoader.load(mChunks[i]->chunk))
              {
                //std::cout << "GEN TERRAIN --> " << mChunks[i]->chunk->pos() << "\n";
                chunkData.resize(cChunk::totalSize * cBlock::dataSize);
                generateChunk(mChunks[i]->chunk->pos(),
                              terrain_t::PERLIN, chunkData );
                //std::cout << "DATA SIZE: --> " << chunkData.size() << "\n";
                mChunks[i]->chunk->deserialize(&chunkData[0], chunkData.size());
                //std::cout << "Deserialize complete!\n";
              }
            else
              {
                //std::cout << "LOADED\n";
              }
            //std::cout << "Setting clean/...\n";
            mChunks[i]->chunk->setClean();
            mChunks[i]->meshDirty = true;
            mChunks[i]->unload = false;
            mChunks[i]->active = true;
            mNumLoaded++;  
            //std::cout << "Setting clean/...\n";            
            //std::cout << "Index: " << i << "\n";
            mChunks[i]->processing.store(false);
          }
        }
      // done processing
      //mChunkCv.notify_one();
    }
}

Point3i cChunkManager::minChunk() const
{ return mMinChunk; }
Point3i cChunkManager::maxChunk() const
{ return mMaxChunk; }

void cChunkManager::setRadius(const Vector3i &loadRadius)
{
  /*
    std::cout << "CHUNK MANAGER SETTING RADIUS\n";
    std::lock_guard<std::mutex> lock(mChunkLock);
  
    Vector3i chunkDim = loadRadius * 2 + 1;
    Vector3i overlapSize({std::min(chunkDim[0], mChunkDim[0]),
    std::min(chunkDim[1], mChunkDim[1]),
    std::min(chunkDim[2], mChunkDim[2]) });
    Point3i offset = (chunkDim - mChunkDim) / 2;
  
    // copy relevant pointers to new chunk array
    std::vector<ChunkData*> newArr(chunkDim[0] * chunkDim[1] * chunkDim[2], nullptr);
    //std::vector<DataRange*> rangeArr(mChunkDim[0]*mChunkDim[1]*mChunkDim[2], nullptr);
    for(int cx = 0; cx < overlapSize[0]; cx++)
    {
    for(int cy = 0; cy < overlapSize[1]; cy++)
    {
    for(int cz = 0; cz < overlapSize[2]; cz++)
    {
    const int oldIndex = index((offset[0] < 0 ? cx - offset[0] : cx),
    (offset[1] < 0 ? cy - offset[1] : cy),
    (offset[2] < 0 ? cz - offset[2] : cz) );
    const int newIndex = index((offset[0] < 0 ? cx : cx + offset[0]),
    (offset[1] < 0 ? cy : cy + offset[1]),
    (offset[2] < 0 ? cz : cz + offset[2]),
    chunkDim[0], chunkDim[2] );
    //std::lock_guard<std::mutex> clock(mChunks[oldIndex]->lock);
    newArr[newIndex] = mChunks[oldIndex];
    }
    }
    }

    // update class parameters
    std::lock_guard<std::mutex> clock(mChunkLock);
    mLoadRadius = loadRadius;
    mChunkDim = loadRadius * 2 + 1;
    mMinChunk = mCenter - mLoadRadius;
    mMaxChunk = mCenter + mLoadRadius;
    mChunks = newArr;
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

void cChunkManager::setCenter(const Point3i &chunkCenter)
{
  Point3i diff = chunkCenter - mCenter;
  if(diff[0] == 0 && diff[1] == 0 && diff[2] == 0)
    { return; }
   
  //std::cout << "Center diff: " << diff << "\n";
  //std::cout << "mChunks.size() --> " << mChunks.size() << "\n";
  mCenterShift = (mCenterShift + diff) % mChunkDim;
  mCenter = chunkCenter;
  mMinChunk = mCenter - mLoadRadius;
  mMaxChunk = mCenter + mLoadRadius;

  for(int i = 0; i < mChunks.size(); i++)
    {
      {
        //std::unique_lock<std::mutex> lock(mChunkLock);
        //mChunkCv.wait(lock, [this, i](){ return !mChunks[i]->processing.exchange(true); });
        //std::cout << "CENTER LOOP\n";
        // chunk is now reserved for processing on this thread.
        if(mChunks[i]->active)
          {
            const Point3i arrayPos = unflattenIndex(i);
            const Point3i relPos = getRelativePos(arrayPos);
            //const Point3i relPos = getRelativePos(mChunks[i]->chunk->pos() - mMinChunk);
            //std::cout << "CHUNK " << i << ": " << arrayPos << " | " << relPos << "\n";
          
            // check if block has gone out of view
            if((diff[0] < 0 && relPos[0] <= -diff[0]-1) ||
               (diff[0] > 0 && relPos[0] >= mChunkDim[0]-diff[0]) ||
               (diff[1] < 0 && relPos[1] <= -diff[1]-1) ||
               (diff[1] > 0 && relPos[1] >= mChunkDim[1]-diff[1]) ||
               (diff[2] < 0 && relPos[2] <= -diff[2]-1) ||
               (diff[2] > 0 && relPos[2] >= mChunkDim[2]-diff[2]) )
              {
                //std::cout << "CHUNK " << i << ": " << relPos << " | " << relPos+mMinChunk << "\n";
                //std::cout << "ACTIVE: " << mChunks[i]->active << ", UNLOAD: " << mChunks[i]->unload << ", MESHDIRTY: " << mChunks[i]->meshDirty << ", DIRTY: " << mChunks[i]->chunk->dirty() << "\n";
                  
                mChunks[i]->unload.store(true);
              }
          }
      }
      // done processing
      //mChunks[i]->processing.store(false);
      //mChunkCv.notify_one();
    }
}


int cChunkManager::numLoaded() const
{
  return mNumLoaded;
}

//cBlock* cChunkManager::get(const Point3i &wp)
block_t cChunkManager::get(const Point3i &wp)
{
  //std::cout << "CHUNK MANAGER GETTING BLOCK\n";
  //std::lock_guard<std::mutex> lock(mChunkLock);
  const int ci = index(getArrayPos(chunkPos(wp) - mMinChunk));
  return ((ci >= 0 && ci < mChunks.size() && mChunks[ci]->active) ?
          mChunks[ci]->chunk->get(cChunk::blockPos(wp)) :
          block_t::NONE );
}
void cChunkManager::set(const Point3i &wp, block_t type)
{
  //std::lock_guard<std::mutex> clock(mChunkLock);
  //std::cout << "CHUNK MANAGER SETTING BLOCK\n";
  const int ci = index(getArrayPos(chunkPos(wp) - mMinChunk));
  if(ci >= 0 && ci < mChunks.size() && mChunks[ci]->active)
    {
      blockSide_t adj = mChunks[ci]->chunk->set(cChunk::blockPos(wp), type);
      std::cout << "ADJACENT: " << (int)adj << "\n";
      mChunks[ci]->meshDirty = true;
    }
}
void cChunkManager::clear()
{
  std::cout << "CHUNK MANAGER CLEARING\n";
  std::lock_guard<std::mutex> lock(mChunkLock);
  for(auto &c : mChunks)
    {
      {
        std::unique_lock<std::mutex> lock(mChunkLock);
        mChunkCv.wait(lock, [&c](){ return !c->processing.exchange(true); });
      
        if(c->active && c->chunk->dirty())
          {                
            if(!mLoader.save(c->chunk))
              { LOGE("Error saving chunk! (skipping)"); }
            c->chunk->setClean();
          }
      }
      c->processing.store(false);
      mChunkCv.notify_one();
    }
  mChunks.clear();
}
void cChunkManager::saveChunks()
{
  std::cout << "CHUNK MANAGER SAVING\n";
  for(auto &c : mChunks)
    {
      {
        std::unique_lock<std::mutex> lock(mChunkLock);
        mChunkCv.wait(lock, [&c](){ return !c->processing.exchange(true); });
      
        if(c->active && c->chunk->dirty())
          {
            if(!mLoader.save(c->chunk))
              { LOGE("Error saving chunk! (skipping)"); }
            c->chunk->setClean();
          }
      }
      c->processing.store(false);
      mChunkCv.notify_one();
    }
}




std::ostream& operator<<(std::ostream &os, const cChunkManager &cm)
{
  os << "<CHUNK MANAGER: " << cm.mChunks.size() << " chunks loaded>";
  return os;
}
  
// optimized functions for indexing
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

inline cBlock* cChunkManager::adjBlock(int ci, int bx, int by, int bz,
				       blockSide_t side )
{
  switch(side)
    {
    case blockSide_t::PX:
      return (bx < cChunk::sizeX-1 ?
	      mChunks[ci]->chunk->at(bx+1, by, bz) :
	      mChunks[adjPX(ci)]->chunk->at(0, by, bz) );
      break;
    case blockSide_t::PY:
      return (by < cChunk::sizeY-1 ?
	      mChunks[ci]->chunk->at(bx, by+1, bz) :
	      mChunks[adjPY(ci)]->chunk->at(bx, 0, bz) );
      break;
    case blockSide_t::PZ:
      return (bz < cChunk::sizeZ-1 ?
	      mChunks[ci]->chunk->at(bx, by, bz+1) :
	      mChunks[adjPZ(ci)]->chunk->at(bx, by, 0) );
    case blockSide_t::NX:
      return (bx > 0 ?
	      mChunks[ci]->chunk->at(bx-1, by, bz) :
	      mChunks[adjNX(ci)]->chunk->at(cChunk::sizeX-1, by, bz) );
    case blockSide_t::NY:
      return (by > 0 ?
	      mChunks[ci]->chunk->at(bx, by-1, bz) :
	      mChunks[adjNY(ci)]->chunk->at(bx, cChunk::sizeY-1, bz) );
    case blockSide_t::NZ:
      return (bz > 0?
	      mChunks[ci]->chunk->at(bx, by, bz-1) :
	      mChunks[adjNZ(ci)]->chunk->at(bx, by, cChunk::sizeZ-1) );
    default:
      return nullptr;
    }
}

inline blockSide_t cChunkManager::adjActive(int ci, int bx, int by, int bz, blockSide_t cSides)
{
  blockSide_t active = blockSide_t::NONE;
  for(int i = 0; i < 6; i++)
    {
      const blockSide_t s = (blockSide_t)(1 << i);
      if((bool)(s & cSides))
	{
	  cBlock *block = adjBlock(ci, bx, by, bz, s);
	  active |= (block && (block->active() && isSimpleBlock(block->type)) ? blockSide_t::NONE : s);
	}
    }
  return active;
}

inline blockSide_t cChunkManager::adjacentLoaded(int ci) const
{
  blockSide_t adjacent = blockSide_t::NONE;
  int ai = adjPX(ci);
  adjacent |= ((ai >= 0 && ai < mChunks.size() && mChunks[ai]->chunk) ?
	       blockSide_t::PX : blockSide_t::NONE );
  ai = adjPY(ci);
  adjacent |= ((ai >= 0 && ai < mChunks.size() && mChunks[ai]->chunk) ?
	       blockSide_t::PY : blockSide_t::NONE );
  ai = adjPZ(ci);
  adjacent |= ((ai >= 0 && ai < mChunks.size() && mChunks[ai]->chunk) ?
	       blockSide_t::PZ : blockSide_t::NONE );
  ai = adjNX(ci);
  adjacent |= ((ai >= 0 && ai < mChunks.size() && mChunks[ai]->chunk) ?
	       blockSide_t::NX : blockSide_t::NONE );
  ai = adjNY(ci);
  adjacent |= ((ai >= 0 && ai < mChunks.size() && mChunks[ai]->chunk) ?
	       blockSide_t::NY : blockSide_t::NONE );
  ai = adjNZ(ci);
  adjacent |= ((ai >= 0 && ai < mChunks.size() && mChunks[ai]->chunk) ?
	       blockSide_t::NZ : blockSide_t::NONE );
  return adjacent;
}


void cChunkManager::generateChunk(const Point3i &chunkPos, terrain_t genType,
                                  std::vector<uint8_t> &dataOut)
{
  switch(genType)
    {
    case terrain_t::PERLIN:
      double height = 1000.0*mNoise.noise((double)chunkPos[0]/100.0, (double)chunkPos[1]/100.0, (double)chunkPos[2]/100.0);
      for(int x = 0; x < cChunk::sizeX; x++)
        for(int y = 0; y < cChunk::sizeY; y++)
          for(int z = 0; z < cChunk::sizeZ; z++)
            {
              int index = cChunk::index(x, y, z);

              Point3i worldPos = Point3i{chunkPos[0]*cChunk::sizeX+x, chunkPos[1]*cChunk::sizeY + y, chunkPos[2]*cChunk::sizeZ + z}*4;
              worldPos[2]*=3;
              Point3i worldPos1 = worldPos / 4;
              Point3i worldPos2 = worldPos * 2;
              Point3i worldPos3 = worldPos / 8;
              /*
              cBlock block;
              
              if(chunkPos[2]*cChunk::sizeZ + z < 4)
                block.type =  block_t::DIRT;//((x + y)%2 ? block_t::DIRT :  block_t::STONE);
              else
                block.type = block_t::NONE;
              
              std::memcpy((void*)&dataOut[index*cBlock::dataSize], (void*)&block.data, cBlock::dataSize);
              
              */
              double n0 = mNoise.noise((double)worldPos[0]/cChunk::sizeX, (double)worldPos[1]/cChunk::sizeY, (double)worldPos[2]/cChunk::sizeZ);
              double n1 = 10.0*mNoise.noise((double)worldPos1[1]/cChunk::sizeX, (double)worldPos1[2]/cChunk::sizeY, (double)worldPos1[0]/cChunk::sizeZ*2);
              double n2 = 5.0*mNoise.noise((double)worldPos2[2]/cChunk::sizeX, (double)worldPos2[0]/cChunk::sizeY, (double)worldPos2[1]/cChunk::sizeZ*3);
              double n3 = 5.0*mNoise.noise((double)worldPos3[2]/cChunk::sizeY, (double)worldPos3[0]/cChunk::sizeZ, (double)worldPos3[1]/cChunk::sizeX*4);
              //std::1cout << height << "\n";
              //n -= 200.0*height;
              //std::cout << "X:" << x << ", Y:" << y << ", Z:" << z << " --> " << n << "\n";

              //n += z*z/(std::abs(y) +1)/(std::abs(x) +1);



              double n = n0 - 2.0*worldPos[2];//mNoise.noise(n0/20, n1/20, n2/20)*1000.0 - worldPos[2];//n0*12 + n1*15 + n2*8 + n3*5 - 10.0;// -  worldPos[2]*5;// : worldPos[2]*worldPos[1];// + n3*worldPos[0] + worldPos[1]*n2;
              
              cBlock b;
              if(n < 0)
                { b.type = block_t::NONE; }
              else if(n < 75.0)
                { b.type = block_t::GRASS; }
              else if(n < 150.0)
                { b.type = block_t::DIRT; }
              else if(n < 350.0)
                { b.type = block_t::SAND; }
              else
                { b.type = block_t::STONE; }
              
              std::memcpy((void*)&dataOut[index*cBlock::dataSize], (void*)&b.data, cBlock::dataSize);
              
            }
      break;
    }
}




/*
// OLD HASHING FUNCTIONS
  
// NOTE: hash is invertible over domain (+/- 2^10)^3
inline int cChunkManager::expand(int x) const
{
//  0000 0000 0000 0000 0000 0011 1111 1111 --> clamp to max value
x                  &= 0x000003FF;
//  1111 1111 0000 0000 0000 0000 1111 1111 --> separate upper 2 bits
x  = (x | (x<<16)) &  0xFF0000FF;
//  0000 1111 0000 0000 1111 0000 0000 1111 --> 
x  = (x | (x<<8))  &  0x0F00F00F;
//  1100 0011 0000 1100 0011 0000 1100 0011 --> 
x  = (x | (x<<4))  &  0xC30C30C3;
//  0100 1001 0010 0100 1001 0010 0100 1001 --> 
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

inline int cChunkManager::chunkHash(int cx, int cy, int cz) const
{ return expand(cx) + (expand(cy) << 1) + (expand(cz) << 2); }
inline int cChunkManager::chunkHash(const Point3i &cp) const
{ return chunkHash(cp[0], cp[1], cp[2]); }
inline int cChunkManager::unhashX(int cx) const
{ return unexpand(cx); }
inline int cChunkManager::unhashY(int cy) const
{ return unexpand(cy); }
inline int cChunkManager::unhashZ(int cz) const
{ return unexpand(cz); }
*/
