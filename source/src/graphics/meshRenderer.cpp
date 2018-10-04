#include "meshRenderer.hpp"

#include "textureAtlas.hpp"
#include "shader.hpp"
#include "meshBuffer.hpp"
#include "params.hpp"
#include "chunk.hpp"
#include "chunkMesh.hpp"
#include "fluidManager.hpp"
#include "world.hpp"
#include "traversalTree.hpp"
#include <unistd.h>



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


void MeshRenderer::startMeshing()
{
  mMeshPool.start();
}
void MeshRenderer::stopMeshing()
{
  mMeshPool.stop(false);
  mMeshCv.notify_all();
  mPriorityCv.notify_all();
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
      
      // mMiniMapShader = new Shader(qParent);
      // if(!mMiniMapShader->loadProgram("./shaders/miniMap.vsh", "./shaders/miniMap.fsh",
      //                                 {"posAttr", "normalAttr", "texCoordAttr"},
      //                                 {"pvm", "uTex"} ))
      //   {
      //     LOGE("MiniMap shader failed to load!");
      //     delete mMiniMapShader;
      //     mMiniMapShader = nullptr;
      //     delete mBlockShader;
      //     mBlockShader = nullptr;
      //     delete mComplexShader;
      //     mComplexShader = nullptr;
      //     return false;
      //   }
      // else
      //   {
      //     mMiniMapShader->bind();
      //     mMiniMapShader->setUniform("uTex", 0);
      //     mMiniMapShader->release();
      //   }

      for(auto &iter : gComplexModelPaths)
        {
          ModelObj *model = new ModelObj(iter.second);
          if(!model->initGL(mComplexShader))
            {
              LOGE("Complex model initialization failed!");
          // delete mMiniMapShader;
          // mMiniMapShader = nullptr;
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
          // delete mMiniMapShader;
          // mMiniMapShader = nullptr;
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
      //delete mMiniMapShader;
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

void MeshRenderer::clearMeshes()
{
  
}

#define RENDER_LOAD_MESH_PER_FRAME 1
void MeshRenderer::render(const Matrix4 &pvm, const Point3f &camPos, bool reset)
{
  if(reset)
    {
      {
        std::lock_guard<std::mutex> lock(mRenderLock);
        for(auto mesh : mRenderMeshes)
          {
            mUnusedMeshes.push(mesh.second);
            std::lock_guard<std::mutex> lock(mMeshedLock);
            mMeshed.erase(mesh.first);
          }
        mRenderMeshes.clear();
      }
      {
        std::lock_guard<std::mutex> lock(mRenderQueueLock);
        while(mRenderQueue.size() > 0)
          {
            MeshedChunk *mc = mRenderQueue.front();
            mRenderQueue.pop();
            std::lock_guard<std::mutex> lock(mUnusedMCLock);
            mUnusedMC.push(mc);
          }
      }
    }
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
            mesh->detachData();
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

  Camera *cam = (mFrustumCulling && mFrustumPaused ? &mPausedCamera : mCamera);
  std::vector<hash_t> visible = mMap->getVisible(mFrustumCulling ?
                                                 (mFrustumPaused ? &mPausedCamera : mCamera) :
                                                 nullptr );
  std::lock_guard<std::mutex> lock(mRenderLock);
  mVisible.clear();
  for(auto hash : visible)
    {
      auto mIter = mRenderMeshes.find(hash);
      if(mIter != mRenderMeshes.end())
        {
          mIter->second->render();
          mVisible.insert(hash);
        }
    }
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
  
  // #define VP_PADDING 20
  // #define VP_W 200
  // #define VP_H 200
  
  // GLint vp[4];
  // glGetIntegerv(GL_VIEWPORT, vp);
  
  // // RENDER MINIMAP
  
  // QMatrix4x4 miniMapP, miniMapV;
  // miniMapP.setToIdentity();
  // miniMapV.setToIdentity();
  
  // miniMapP.ortho(camPos[0]-(mRadius[0]-1)*Chunk::sizeX, camPos[0]+(mRadius[0]+1)*Chunk::sizeX,
  //                camPos[1]-(mRadius[1]-1)*Chunk::sizeY, camPos[1]+(mRadius[1]+1)*Chunk::sizeY, 50, -50 );
  // miniMapV.lookAt(toQt(camPos),
  //                 QVector3D(camPos[0], camPos[1], camPos[2]-1),
  //                 QVector3D(0.0f,1.0f,0.0f) );

  // mMiniMapShader->bind();
  // glViewport(VP_PADDING, VP_PADDING, VP_W, VP_H);
  // glClear(GL_DEPTH_BUFFER_BIT);
  // mMiniMapShader->setUniform("pvm", Matrix4(miniMapP) * Matrix4(miniMapV));
  // for(auto mesh : mRenderMeshes)
  //   { mesh.second->render(); }
  // glViewport(vp[0], vp[1], vp[2], vp[3]);
  // mMiniMapShader->release();
    
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
  mCenter = newCenter;
}

void MeshRenderer::load(Chunk *chunk, const Point3i &center, bool priority)
{
  mCenter = center;
  {
    std::lock_guard<std::mutex> lock(mChunkLock);
    mRenderChunks[Hash::hash(chunk->pos())] = chunk;
  }
  
  std::unique_lock<std::mutex> lock(mMeshLock);
  if(priority)
    { mPriorityMeshQueue.push_back(chunk); }
  else
    { mMeshQueue.push_back(chunk); }
  
  mMeshing.insert(Hash::hash(chunk->pos()));
  lock.unlock();
  if(priority)
    {
      mPriorityCv.notify_one();
    }
  else
    {
      mMeshCv.notify_one();
    }
}
void MeshRenderer::unload(hash_t hash)
{
  { // stop meshing chunk
    std::lock_guard<std::mutex> lock(mMeshLock);
    mMeshing.erase(hash);
  }
  { // remove complex chunk
    std::lock_guard<std::mutex> lock(mRenderLock);
    mComplexBlocks.erase(hash);
  }
  { // unload mesh
    std::lock_guard<std::mutex> lock(mUnloadLock);
    mUnloadQueue.insert(hash);
  }
}

bool MeshRenderer::isMeshed(hash_t hash)
{
  std::unique_lock<std::mutex> lock(mMeshedLock);
  return (mMeshed.find(hash) != mMeshed.end());
}
bool MeshRenderer::isMeshing(hash_t hash)
{
  std::unique_lock<std::mutex> lock(mMeshedLock);
  return (mMeshingNow.find(hash) != mMeshingNow.end());
}

bool MeshRenderer::isVisible(hash_t hash)
{
  std::unique_lock<std::mutex> lock(mRenderLock);
  return mVisible.count(hash) > 0;
}

void MeshRenderer::meshWorker(int tid)
{
  if(!mMeshPool.running())
    { return; }
  
  // wait for chunks to mesh
  std::unique_lock<std::mutex> lock(mMeshLock);
  mWaitingThreads++;
  ChunkPtr next = nullptr;
  mMeshCv.wait(lock, [this]{ return (!mMeshPool.running() ||
                                     mPriorityMeshQueue.size() > 0 ||
                                     mMeshQueue.size() > 0 );});
  if(!mMeshPool.running())
    { return; }

  if(mPriorityMeshQueue.size() > 0)
    {
      next = mPriorityMeshQueue.front();
      mPriorityMeshQueue.pop_front();
    }
  else if(mMeshQueue.size() > 0)
    {
      next = mMeshQueue.front();
      mMeshQueue.pop_front();
    }
    
  mWaitingThreads--;
  
  // get chunk
  hash_t cHash = next->hash();
  if(mMeshing.count(cHash) > 0)
    { // mesh chunk
      mMeshing.erase(cHash);
      //LOGD("MESHING: %d", cHash);
      lock.unlock();
      {
        std::lock_guard<std::mutex> lock(mMeshedLock);
        mMeshingNow.insert(cHash);
      }
      updateChunkMesh(next);
      {
        std::lock_guard<std::mutex> lock(mMeshedLock);
        mMeshingNow.erase(cHash);
      }
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
  const Point3i cPos = chunk->pos();
  const hash_t cHash = Hash::hash(cPos);
  if(chunk->isEmpty())
    {
      unload(cHash);
      return;
    }
  
  chunk->calcBounds();
  ChunkBounds *bounds = chunk->getBounds();
  bool hasFluids = mFluids->setChunkBoundary(cHash, bounds);
  
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

  const Point3i minP({cPos[0]*Chunk::sizeX,
                      cPos[1]*Chunk::sizeY,
                      cPos[2]*Chunk::sizeZ} );

  bounds->lock();
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
                  if(vn < 2)
                    { sum0 += lighting; }
                  else
                    { sum1 += lighting; }
                      
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
}

void MeshRenderer::setCamera(Camera *camera)
{ mCamera = camera; }
void MeshRenderer::setFrustumCulling(bool on)
{ mFrustumCulling = on; }
void MeshRenderer::pauseFrustumCulling()
{
  if(mFrustumCulling)
    {
      std::lock_guard<std::mutex> lock(mRenderLock);
      if(!mFrustumPaused)
        {
          mFrustumPaused = true;
          mPausedCamera = *mCamera;
          std::cout << "FRUSTUM PAUSED\n";
        }
      else
        {
          mFrustumPaused = false;
          std::cout << "FRUSTUM UNPAUSED\n";
        }
    }
}


MeshData MeshRenderer::makeFrustumMesh()
{
  return mPausedCamera.makeDebugMesh();
}
