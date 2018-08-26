#include "world.hpp"
#include "timing.hpp"
#include <unistd.h>
#include <random>

cWorld::cWorld(const std::string &worldName, uint32_t seed, int loadThreads,
	       const Point3i &center, const Vector3i &chunkRadius )
  : mChunks(loadThreads, center, chunkRadius)
//: mNumThreads(numThreads), mThreadChunks(numThreads),
    //mThreadFaces(numThreads), mThreadComplex(numThreads)
{
  //mThreads.reserve(mNumThreads);
  //for(int i = 0; i < mNumThreads; i++)
  //  { mThreads.emplace_back(&cWorld::chunkWorker, this, i); }
  
  if(!mChunks.setWorld(worldName))
    {
      exit(1);
    }
}

cWorld::~cWorld()
{
  mChunks.stop();
  //mThreadsRunning = false;
  //{
  //  std::lock_guard<std::mutex> lock(mThreadWorkLock);
  //  mThreadsWorking = (1 << mNumThreads) - 1;
  //}
  //mThreadWorkCv.notify_all();
  
  // for(auto &t : mThreads)
  //   { t.join(); }
}

block_t cWorld::get(const Point3i &worldPos)
{
  
  //LOGD("WORLD GETTING");
  return mChunks.get(worldPos);
  // if(mData.cellMapped({worldPos[0] / CHUNK_X, worldPos[1] / CHUNK_Y, worldPos[2] / CHUNK_Z}))
  //   { return mData.get(worldPos); }
  // else
  //   { return block_t::NONE; }
}

bool cWorld::set(const Point3i &worldPos, block_t type)
{
  //LOGD("WORLD SETTING");
  block_t b = get(worldPos);
  if((b == block_t::NONE) != (type == block_t::NONE))
    {
      if(b != type)
	{
	  mChunks.set(worldPos, type);
	  //if(mInitialized)
	  //{ mNeedUpdate = true; }
	}
      return true;
    }
  else
    { return false; }
}

cChunk* cWorld::getChunk(const Point3i &worldPos)
{
  LOGW("GETCHUNK CALLED --> REMOVE!!");
  return nullptr; //mData.getChunkAt(worldPos);
}

// Ray casting
static float mod(float value, float modulus)
{
  return std::fmod(value, modulus);
}

static float intbound(float s, float ds)
{
  return (ds > 0 ?
	  (std::floor(s) + 1 - s) / std::abs(ds) :
	  (ds <= 0 ? (s - std::floor(s)) / std::abs(ds) : 0));
}

bool cWorld::rayCast(const Point3f &p, const Vector3f &d, float radius,
		     cBlock* &blockOut, Point3i &posOut, Vector3i &faceOut )
{
  //LOGD("RAYCASTING");
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
  Point3i chunkMin = mChunks.minChunk();
  Point3i chunkMax = mChunks.maxChunk() + Point3i{1, 1, 1};
  
  while((step[0] > 0 ? (pi[0] < chunkMax[0]*cChunk::sizeX) : (pi[0] >= chunkMin[0]*cChunk::sizeX)) &&
	(step[1] > 0 ? (pi[1] < chunkMax[1]*cChunk::sizeY) : (pi[1] >= chunkMin[1]*cChunk::sizeY)) &&
	(step[2] > 0 ? (pi[2] < chunkMax[2]*cChunk::sizeZ) : (pi[2] >= chunkMin[2]*cChunk::sizeZ)) )
    {
      if(!(pi[0] < chunkMin[0]*cChunk::sizeX ||
	   pi[1] < chunkMin[1]*cChunk::sizeY ||
	   pi[2] < chunkMin[2]*cChunk::sizeZ ||
	   pi[0] >= chunkMax[0]*cChunk::sizeX ||
	   pi[1] >= chunkMax[1]*cChunk::sizeY ||
	   pi[2] >= chunkMax[2]*cChunk::sizeZ ))
	{
	  cBlock *b = mChunks.at(pi);
	  if(b && b->type != block_t::NONE)
	    {
	      blockOut = b;
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



void cWorld::initGL(QObject *qparent)
{
  mChunks.initGL(qparent);
}

void cWorld::cleanupGL()
{
  mChunks.cleanupGL();
}

void cWorld::render(Matrix4 pvm)
{
  mChunks.render(pvm);
}


void cWorld::update()
{
  mChunks.update();

  //mPlayer.update(dt);
}

bool cWorld::readyForPlayer() const
{
  return (mChunks.numLoaded() > 0);
}

Point3f cWorld::getStartPos(const Point3i &pPos)
{
  return Point3f{pPos[0], pPos[1], pPos[2]};//mChunks.getHeightAt(pPos)};
}


Point3i cWorld::chunkPos(const Point3f &worldPos) const
{
  return cChunkManager::chunkPos(Point3i{std::floor(worldPos[0]),
                                         std::floor(worldPos[1]),
                                         std::floor(worldPos[2]) });
}

void cWorld::setCenter(const Point3i &chunkCenter)
{
  mChunks.setCenter(chunkCenter);
}
Point3i cWorld::getCenter() const
{ return mChunks.getCenter(); }

void cWorld::startLoading()
{
  mChunks.start();
  //usleep(1000000);
}
void cWorld::stopLoading()
{
  mChunks.stop();
}











/*
void cWorld::loadModels()
{
  mModels[complexIndex(block_t::DEVICE)] = cModelObj("./res/device.obj");
  mModels[complexIndex(block_t::CPU)] = cModelObj("./res/cpu.obj");
  mModels[complexIndex(block_t::MEMORY)] = cModelObj("./res/memory.obj");
  mModels[complexIndex(block_t::LIGHT)] = cModelObj("./res/cube.obj");
  
  for(int i = 0; i < (int)normal_t::COUNT; i++)
    {
      Vector3f norm;
      Point3f minP;
      Point3f maxP;
      Point3f midP1;
      Point3f midP2;
      switch((normal_t)i)
	{
	case normal_t::PX:
	  norm[0] = 1;
	  minP = Point3i{1, 0, 0};
	  maxP = Point3i{1, 1, 1};
	  midP1 = Point3i{1, 0, 1};
	  midP2 = Point3i{1, 1, 0};
	  break;
	case normal_t::PY:
	  norm[1] = 1;
	  minP = Point3i{0, 1, 0};
	  maxP = Point3i{1, 1, 1};
	  midP1 = Point3i{1, 1, 0};
	  midP2 = Point3i{0, 1, 1};
	  break;
	case normal_t::PZ:
	  norm[2] = 1;
	  minP = Point3i{0, 0, 1};
	  maxP = Point3i{1, 1, 1};
	  midP1 = Point3i{0, 1, 1};
	  midP2 = Point3i{1, 0, 1};
	  break;
	case normal_t::NX:
	  norm[0] = -1;
	  minP = Point3i{0, 0, 0};
	  maxP = Point3i{0, 1, 1};
	  midP1 = Point3i{0, 1, 0};
	  midP2 = Point3i{0, 0, 1};
	  break;
	case normal_t::NY:
	  norm[1] = -1;
	  minP = Point3i{0, 0, 0};
	  maxP = Point3i{1, 0, 1};
	  midP1 = Point3i{0, 0, 1};
	  midP2 = Point3i{1, 0, 0};
	  break;
	case normal_t::NZ:
	  norm[2] = -1;
	  minP = Point3i{0, 0, 0};
	  maxP = Point3i{1, 1, 0};
	  midP1 = Point3i{1, 0, 0};
	  midP2 = Point3i{0, 1, 0};
	  break;
	}
      mFaceVertices[i][0] = cSimpleVertex(minP, norm, Vector2f{0.0f, 0.0f} );
      mFaceVertices[i][1] = cSimpleVertex(maxP, norm, Vector2f{1.0f, 1.0f} );
      mFaceVertices[i][2] = cSimpleVertex(midP1, norm, Vector2f{0.0f, 1.0f} );
      mFaceVertices[i][3] = cSimpleVertex(midP2, norm, Vector2f{1.0f, 0.0f} );
    }
  
  mFaceIndices = { 0, 1, 2, 3, 1, 0 };
  //mFaceIndices[1] = { 0, 1, 2, 3, 1, 0 }; // negative normals
}

void cWorld::modelInitGL(cShader *complexShader)
{
  for(auto &m : mModels)
    { m.initGL(complexShader); }
}
void cWorld::modelCleanupGL()
{
  for(auto &m : mModels)
    { m.cleanupGL(); }
}
*/
