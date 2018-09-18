#include "meshRenderer.hpp"

#include "textureAtlas.hpp"
#include "shader.hpp"
#include "meshBuffer.hpp"
#include "params.hpp"
#include "chunk.hpp"
#include "chunkMesh.hpp"
#include "fluidManager.hpp"
#include "frustum.hpp"
#include "world.hpp"



// CUBE FACE INDICES
const std::array<unsigned int, 6> MeshRenderer::faceIndices =
  { 0, 1, 2, 3, 1, 0 };
const std::array<unsigned int, 6> MeshRenderer::reverseIndices =
  { 0, 1, 3, 2, 1, 0 };
const std::array<unsigned int, 6> MeshRenderer::flippedIndices =
  { 0, 3, 2, 1, 2, 3 };
std::unordered_map<blockSide_t, std::array<cSimpleVertex, 4>> MeshRenderer::faceVertices =
  // CUBE FACE VERTICES
  {{blockSide_t::PX,
    {cSimpleVertex(Point3f{1, 0, 0}, Vector3f{1, 0, 0}, Vector2f{0.0f, 0.0f} ),
     cSimpleVertex(Point3f{1, 1, 1}, Vector3f{1, 0, 0}, Vector2f{1.0f, 1.0f} ),
     cSimpleVertex(Point3f{1, 0, 1}, Vector3f{1, 0, 0}, Vector2f{0.0f, 1.0f} ),
     cSimpleVertex(Point3f{1, 1, 0}, Vector3f{1, 0, 0}, Vector2f{1.0f, 0.0f} ) }},
   {blockSide_t::PY,
    {cSimpleVertex(Point3f{0, 1, 0}, Vector3f{0, 1, 0}, Vector2f{0.0f, 0.0f} ),
     cSimpleVertex(Point3f{1, 1, 1}, Vector3f{0, 1, 0}, Vector2f{1.0f, 1.0f} ),
     cSimpleVertex(Point3f{1, 1, 0}, Vector3f{0, 1, 0}, Vector2f{0.0f, 1.0f} ),
     cSimpleVertex(Point3f{0, 1, 1}, Vector3f{0, 1, 0}, Vector2f{1.0f, 0.0f} ) }},
   {blockSide_t::PZ,
    {cSimpleVertex(Point3f{0, 0, 1}, Vector3f{0, 0, 1}, Vector2f{0.0f, 0.0f} ),
     cSimpleVertex(Point3f{1, 1, 1}, Vector3f{0, 0, 1}, Vector2f{1.0f, 1.0f} ),
     cSimpleVertex(Point3f{0, 1, 1}, Vector3f{0, 0, 1}, Vector2f{0.0f, 1.0f} ),
     cSimpleVertex(Point3f{1, 0, 1}, Vector3f{0, 0, 1}, Vector2f{1.0f, 0.0f} ) }},
   {blockSide_t::NX,
    {cSimpleVertex(Point3f{0, 0, 0}, Vector3f{-1, 0, 0}, Vector2f{0.0f, 0.0f} ),
     cSimpleVertex(Point3f{0, 1, 1}, Vector3f{-1, 0, 0}, Vector2f{1.0f, 1.0f} ),
     cSimpleVertex(Point3f{0, 1, 0}, Vector3f{-1, 0, 0}, Vector2f{0.0f, 1.0f} ),
     cSimpleVertex(Point3f{0, 0, 1}, Vector3f{-1, 0, 0}, Vector2f{1.0f, 0.0f} ) }},
   {blockSide_t::NY,
    {cSimpleVertex(Point3f{0, 0, 0}, Vector3f{0, -1, 0}, Vector2f{0.0f, 0.0f} ),
     cSimpleVertex(Point3f{1, 0, 1}, Vector3f{0, -1, 0}, Vector2f{1.0f, 1.0f} ),
     cSimpleVertex(Point3f{0, 0, 1}, Vector3f{0, -1, 0}, Vector2f{0.0f, 1.0f} ),
     cSimpleVertex(Point3f{1, 0, 0}, Vector3f{0, -1, 0}, Vector2f{1.0f, 0.0f} ) }},
   {blockSide_t::NZ,
    {cSimpleVertex(Point3f{0, 0, 0}, Vector3f{0, 0, -1}, Vector2f{0.0f, 0.0f} ),
     cSimpleVertex(Point3f{1, 1, 0}, Vector3f{0, 0, -1}, Vector2f{1.0f, 1.0f} ),
     cSimpleVertex(Point3f{1, 0, 0}, Vector3f{0, 0, -1}, Vector2f{0.0f, 1.0f} ),
     cSimpleVertex(Point3f{0, 1, 0}, Vector3f{0, 0, -1}, Vector2f{1.0f, 0.0f} ) }} };

const std::array<blockSide_t, 6> MeshRenderer::meshSides =
  { blockSide_t::PX, blockSide_t::PY,
    blockSide_t::PZ, blockSide_t::NX,
    blockSide_t::NY, blockSide_t::NZ };
const std::array<Point3i, 6> MeshRenderer::meshSideDirections =
  { Point3i{1,0,0}, Point3i{0,1,0}, Point3i{0,0,1},
    Point3i{-1,0,0}, Point3i{0,-1,0}, Point3i{0,0,-1} };



MeshRenderer::MeshRenderer()
  : mMeshPool(1, std::bind(&MeshRenderer::meshWorker, this, std::placeholders::_1),
              MESH_THREAD_SLEEP_MS*1000)
{

}

MeshRenderer::~MeshRenderer()
{
  
}

void MeshRenderer::setMeshThreads(int meshThreads)
{
  mMeshPool.setThreads(meshThreads);
}
void MeshRenderer::start()
{
  mMeshPool.start();
}
void MeshRenderer::stop()
{
  mMeshPool.stop(false);
  mMeshCv.notify_all();
  mMeshPool.stop(true);
}

int MeshRenderer::numMeshed()
{
  int num = 0;
  std::lock_guard<std::mutex> lock(mMeshedLock);
  return mMeshed.size();
}

bool MeshRenderer::initGL(QObject *qParent)
{
  if(!mInitialized)
    {
      // load shaders
      mBlockShader = new Shader(qParent);
      if(!mBlockShader->loadProgram("./shaders/simpleBlock.vsh", "./shaders/simpleBlock.fsh",
                                    {"posAttr", "normalAttr", "texCoordAttr"},
                                    {"pvm", "camPos", "fogStart", "fogEnd", "uTex", "dirScale"} ))
        {
          LOGE("Simple block shader failed to load!");
          delete mBlockShader;
          mBlockShader = nullptr;
          return false;
        }
      else
        {
          mBlockShader->bind();
          mBlockShader->setUniform("uTex", 0);
          mBlockShader->setUniform("fogStart", mFogStart);
          mBlockShader->setUniform("fogEnd", mFogEnd);
          mBlockShader->setUniform("dirScale", mDirScale);
          mBlockShader->release();
        }

      mComplexShader = new Shader(qParent);
      if(!mComplexShader->loadProgram("./shaders/complexBlock.vsh", "./shaders/complexBlock.fsh",
                                      {"posAttr", "normalAttr",  "texCoordAttr"},
                                      {"pvm", "uTex", "uBlockType"} ))
        {
          LOGE("Complex shader failed to load!");
          delete mComplexShader;
          mComplexShader = nullptr;
          delete mBlockShader;
          mBlockShader = nullptr;
          return false;
        }
      else
        {
          mComplexShader->bind();
          mComplexShader->setUniform("uTex", 0);
          mComplexShader->release();
        }

      for(auto &iter : gComplexModelPaths)
        {
          ModelObj *model = new ModelObj(iter.second);
          if(!model->initGL(mComplexShader))
            {
              LOGE("Complex model initialization failed!");
              delete mComplexShader;
              mComplexShader = nullptr;
              delete mBlockShader;
              mBlockShader = nullptr;
              return false;
            }
          mComplexModels.emplace(iter.first, model);
        }
      
      // load block textures
      mTexAtlas = new cTextureAtlas(qParent, ATLAS_BLOCK_SIZE);
      if(!mTexAtlas->create("./res/texAtlas.png"))
        {
          LOGE("Failed to load texture atlas!");
          delete mComplexShader;
          mComplexShader = nullptr;
          delete mBlockShader;
          mBlockShader = nullptr;
          delete mTexAtlas;
          mTexAtlas = nullptr;
          return false;
        }
      
      mInitialized = true;
    }
  return true;
}

void MeshRenderer::cleanupGL()
{
  if(mInitialized)
    {
      delete mBlockShader;
      mTexAtlas->destroy();
      delete mTexAtlas;

      for(auto &iter : mComplexModels)
        {
          iter.second->cleanupGL();
          delete iter.second;
        }
      mComplexModels.clear();
      delete mComplexShader;
      mComplexShader = nullptr;
      
      {
        std::lock_guard<std::mutex> lock(mRenderLock);
        for(auto mesh : mRenderMeshes)
          {
            mesh.second->cleanupGL();
            delete mesh.second;
          }
        mRenderMeshes.clear();
      }
      while(mUnusedMeshes.size() > 0)
        {
          ChunkMesh *mesh = mUnusedMeshes.front();
          mUnusedMeshes.pop();
          mesh->cleanupGL();
          delete mesh;
        }
      std::lock_guard<std::mutex> lock(mUnusedMCLock);
      while(mUnusedMC.size() > 0)
        {
          MeshedChunk *mc = mUnusedMC.front();
          mUnusedMC.pop();
          delete mc;
        }
      mInitialized = false;
    }
}


void MeshRenderer::addMesh(MeshedChunk *mc)
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

#define RENDER_LOAD_MESH_PER_FRAME 2
void MeshRenderer::render(const Matrix4 &pvm, const Point3f &camPos)
{
  {
    std::lock_guard<std::mutex> lock(mRenderQueueLock);
    int num = 0;
    while(mRenderQueue.size() > 0)
      {
        MeshedChunk *mc = mRenderQueue.front();
        mRenderQueue.pop();
        addMesh(mc);
        
        if(++num >= RENDER_LOAD_MESH_PER_FRAME)
          { break; }
      }
    // update fluid meshes
    auto updates = mFluids->getUpdates();
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
    std::lock_guard<std::mutex> lock(mUnloadLock);
    for(auto hash : mUnloadQueue)
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
    mUnloadQueue.clear();
  }

  // render all loaded chunks
  mTexAtlas->bind();
  {
    std::lock_guard<std::mutex> lock(mRenderLock);
    mBlockShader->bind();
    mBlockShader->setUniform("pvm", pvm);
    mBlockShader->setUniform("camPos", camPos);
    if(mFogChanged)
      {
        mBlockShader->setUniform("fogStart", mFogStart);
        mBlockShader->setUniform("fogEnd", mFogEnd);
        mBlockShader->setUniform("dirScale", mDirScale);
        mFogChanged = false;
      }
    for(auto &iter : mRenderMeshes)
      { iter.second->render(); }
    for(auto &iter : mFluidMeshes)
      { iter.second->render(); }
    mBlockShader->release();

    // render complex models
    mComplexShader->bind();
    auto indexer = Chunk::indexer();
    {
      for(auto &iter : mComplexBlocks)
        {
          Point3i cp = Hash::unhash(iter.first);
          for(auto &c : iter.second)
            {
              Point3i bp = cp*Chunk::size + indexer.unindex(c.first);
              Matrix4 pvmTrans = pvm;
              pvmTrans.translate(bp[0], bp[1], bp[2]);
              mComplexShader->setUniform("uBlockType", (int)c.second->type()-1);
              mComplexModels[c.second->type()]->render(mComplexShader, pvm*pvmTrans);
            }
        }
    }
    mComplexShader->release();
  }
  mTexAtlas->release();
}


void MeshRenderer::setFog(float fogStart, float fogEnd, const Vector3f &dirScale)
{
  std::lock_guard<std::mutex> lock(mRenderLock);
  mFogStart = fogStart;
  mFogEnd = fogEnd;
  mDirScale = dirScale;
  mFogChanged = true;
}

void MeshRenderer::reorderQueue(const Point3i &newCenter)
{
  std::unique_lock<std::mutex> lock(mMeshLock);
  auto newQueue = mLoadQueue;
  mLoadQueue.clear();

  Vector3i diff;
  for(auto chunk : newQueue)
    {
      diff = chunk->pos() - newCenter;
      const int cDist = diff.dot(diff);

      auto iter = mLoadQueue.begin();
      for(; iter != mLoadQueue.end(); ++iter)
        {
          diff = (*iter)->pos() - newCenter;
          if(diff.dot(diff) >= cDist)
            { break; }
        }
      mLoadQueue.insert(iter, chunk);
    }
  lock.unlock();
  mMeshCv.notify_all();
}

void MeshRenderer::load(Chunk *chunk, const Point3i &center)
{
  Vector3i diff = chunk->pos() - center;
  const hash_t hash = Hash::hash(chunk->pos());
  const int cDist = diff.dot(diff);
  
  std::unique_lock<std::mutex> lock(mMeshLock);
  if(mMeshing.count(hash) == 0)
    {
      auto iter = mLoadQueue.begin();
      for(; iter != mLoadQueue.end(); ++iter)
        {
          diff = (*iter)->pos() - center;
          if(diff.dot(diff) >= cDist)
            { break; }
        }
      mLoadQueue.insert(iter, chunk);
      mMeshing.insert(hash);
    }
  lock.unlock();
  mMeshCv.notify_one();
}
void MeshRenderer::unload(hash_t hash)
{
  { // stop meshing chunk
    std::lock_guard<std::mutex> lock(mMeshLock);
    if(mMeshing.count(hash) > 0)
      { mMeshing.erase(hash); }
  }
  {
    std::lock_guard<std::mutex> lock(mRenderLock);
    mComplexBlocks.erase(hash);
  }
  // unload mesh
  std::lock_guard<std::mutex> lock(mUnloadLock);
  mUnloadQueue.insert(hash);
}

bool MeshRenderer::isMeshed(hash_t hash)
{
  std::unique_lock<std::mutex> lock(mMeshLock);
  auto mIter = mMeshed.find(hash);
  return (mIter != mMeshed.end());
}

void MeshRenderer::meshWorker(int tid)
{
  if(!mMeshPool.running())
    { return; }
  // wait for chunks to mesh
  std::unique_lock<std::mutex> lock(mMeshLock);
  mMeshCv.wait(lock, [this]{ return !mMeshPool.running() || mLoadQueue.size() > 0; });
  if(!mMeshPool.running())
    { return; }

  // get chunk
  Chunk *next = mLoadQueue.front();
  mLoadQueue.pop_front();
  hash_t cHash = Hash::hash(next->pos());
  if(mMeshing.count(cHash) > 0)
    { // mesh chunk
      mMeshing.erase(cHash);
      mMeshLock.unlock();
      updateChunkMesh(next);
    }
}

inline int MeshRenderer::getAO(int e1, int e2, int c)
{
  if(e1 == 1 && e2 == 1)
    { return 3; }
  else
    { return (e1 + e2 + c); }
}

block_t MeshRenderer::getBlock(Chunk *chunk, const Point3i &wp)
{
  Point3i diff = World::chunkPos(wp) - chunk->pos();
  if(diff[0] != 0 || diff[1] != 0 || diff[2] != 0)
    {
      Chunk *n = chunk->getNeighbor(getSide(diff));
      if(n)
        { return n->getType(Chunk::blockPos(wp)); }
      else
        { return block_t::NONE; }
    }
  else
    { return chunk->getType(Chunk::blockPos(wp)); }
}

int MeshRenderer::getLighting(Chunk *chunk, const Point3i &wp, const Point3f &vp, blockSide_t side)
{
  const int dim = sideDim(side); // dimension of face normal
  const int dim1 = (dim+1) % 3;  // dimension of edge 1
  const int dim2 = (dim+2) % 3;  // dimension of edge 2

  const int e1 = wp[dim1] + (int)vp[dim1] * 2 - 1;
  const int e2 = wp[dim2] + (int)vp[dim2] * 2 - 1;
  
  Point3i bi;
  bi[dim] = wp[dim] + sideSign(side);
  bi[dim1] = e1;
  bi[dim2] = e2;

  Chunk *nc = nullptr;
  int lc = (isSimpleBlock(getBlock(chunk, bi)) ? 1 : 0);
  bi[dim1] = wp[dim1];
  int le1 = (isSimpleBlock(getBlock(chunk, bi)) ? 1 : 0);
  bi[dim1] = e1;
  bi[dim2] = wp[dim2];
  int le2 = (isSimpleBlock(getBlock(chunk, bi)) ? 1 : 0);
  return 3 - getAO(le1, le2, lc);
}


// 10ms avg
void MeshRenderer::updateChunkMesh(Chunk *chunk)
{
  auto start = std::chrono::high_resolution_clock::now();
  if(chunk->isEmpty())
    { return; }
  
  const Point3i cPos = chunk->pos();
  const hash_t cHash = Hash::hash(cPos);
  
  bool empty = !chunk->calcBounds();
  ChunkBounds *bounds = chunk->getBounds();
  bool hasFluids = mFluids->setChunkBoundary(cHash, bounds);

  double t = (std::chrono::high_resolution_clock::now() - start).count();

  { // 4ms avg
    std::lock_guard<std::mutex> lock(mTimingLock);
    mBoundsTime += t;
    mBoundsNum++;
    if(mBoundsNum >= 16)
      {
        LOGD("Avg Bounds Time: %f", mBoundsTime / mBoundsNum / 1000000000.0);
        mBoundsTime = 0.0;
        mBoundsNum = 0;
      }
  }
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

      start = std::chrono::high_resolution_clock::now();
      const Point3i minP({cPos[0]*Chunk::sizeX,
                          cPos[1]*Chunk::sizeY,
                          cPos[2]*Chunk::sizeZ} );
      const Point3i maxP = minP + Chunk::size;

      //Point3i diff = chunk->pos() - mCenter;
      //float dist = sqrt(diff.dot(diff));
  
      bounds->lock();
      if(true)//dist < 2.0)
        {
          std::unordered_map<hash_t, ActiveBlock> &chunkBounds = bounds->getBounds();
          for(auto &iter : chunkBounds)
            {
              const hash_t hash = iter.first;
              const ActiveBlock &block = iter.second;
          
              const Point3i bp = Hash::unhash(hash);
              const Point3i vOffset = minP + bp;
          
              for(int i = 0; i < 6; i++)
                {
                  if((block.sides & meshSides[i]) != blockSide_t::NONE)
                    {
                      int vn = 0;
                      int sum0 = 0;
                      int sum1 = 0;
                      const unsigned int numVert = mc->mesh.vertices().size();
                      for(auto &v : faceVertices[meshSides[i]])
                        { // add vertices for this face
                          const int lighting = getLighting(chunk, vOffset, v.pos, meshSides[i]);
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
        }
      else
        {
          std::unordered_map<hash_t, ActiveBlock> &chunkBounds = bounds->getBounds();
          for(auto &iter : chunkBounds)
            {
              const hash_t hash = iter.first;
              const ActiveBlock &block = iter.second;
          
              const Point3i bp = Hash::unhash(hash);
              const Point3i vOffset = minP + bp;

              const unsigned int numVert = mc->mesh.vertices().size();
                  
              //const int lighting = getLighting(chunk, vOffset, Point3f{0,0,0}, blockSide_t::NONE);
              //sum0 += (vn < 2) ? lighting : 0;
              //sum1 += (vn < 2) ? 0 : lighting;

              if((bp[0]%2 + bp[1]%2 + bp[2]%2) != 0)
                {
                  mc->mesh.vertices().emplace_back(vOffset,// + v.pos,
                                                   Vector3f{0,0,1},
                                                   Vector2f{0.5, 0.5},
                                                   (int)block.block - 1,
                                                   (float)4 / (float)4 );
                }
              //mc->mesh.indices().push_back(numVert);
              /*
                for(int i = 0; i < 6; i++)
                {
                if((bool)(block.sides & meshSides[i]))
                {
                int vn = 0;
                int sum0 = 0;
                int sum1 = 0;
                const unsigned int numVert = mc->mesh.vertices().size();
                for(auto &v : faceVertices[meshSides[i]])
                { // add vertices for this face
                vn++;
                }
                const std::array<unsigned int, 6> *orientedIndices = (sum1 > sum0 ?
                &flippedIndices :
                &faceIndices );
                for(auto i : *orientedIndices)
                { mc->mesh.indices().push_back(numVert + i); }
                }
                }
              */
            }

          for(int i = 0; i < mc->mesh.vertices().size(); i++)
            {
              std::vector<int> close;
              auto &v = mc->mesh.vertices()[i];
              float minDist = 100000000.0f;
              int minI = i;
              float minDist2 = 1000000000.0f;
              int minI2 = i;
              for(int j = 0; j < mc->mesh.vertices().size(); j++)
                {
                  auto &v2 = mc->mesh.vertices()[j];
                  Point3i vDiff = v.pos - v2.pos;
                  float vDist = vDiff.dot(vDiff);
                  if(vDist < 6)
                    {
                      close.push_back(j);
                    }
                }

              for(int j = 1; j < close.size(); j++)
                {
                  mc->mesh.indices().push_back(close[j]);
                  mc->mesh.indices().push_back(close[j-1]);
                  mc->mesh.indices().push_back(i);
                }
            }
              
        }
      bounds->unlock();
      {
        std::lock_guard<std::mutex> lock(mRenderLock);
        mComplexBlocks.erase(cHash);
        mComplexBlocks.emplace(cHash, chunk->getComplex());
      }
      // pass to render thread
      {
        std::lock_guard<std::mutex> lock(mRenderQueueLock);
        mRenderQueue.push(mc);
      }
      t = (std::chrono::high_resolution_clock::now() - start).count();

      // 7ms avg
      std::lock_guard<std::mutex> lock(mTimingLock);
      mMeshTime += t;
      mMeshNum++;
      if(mMeshNum >= 16)
        {
          LOGD("Avg Mesh Time: %f", mMeshTime / mMeshNum / 1000000000.0);
          mMeshTime = 0.0;
          mMeshNum = 0;
        }
    }
}

void MeshRenderer::setFrustum(Frustum *frustum)
{ mFrustum = frustum; }
void MeshRenderer::setFrustumClip(bool on)
{ mClipFrustum = on; }

void MeshRenderer::setFrustumPause()
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
            { mFrustumRender.insert(iter.first); num++; }
          else
            { mFrustumRender.erase(iter.first); }
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
