#include "rayTracer.hpp"

#include "textureAtlas.hpp"
#include "shader.hpp"
#include "computeShader.hpp"
#include "camera.hpp"
#include "chunk.hpp"
#include "world.hpp"
#include "quadBuffer.hpp"

RayTracer::RayTracer()
  : mScreenSize({1024, 1024})
{
  
}

RayTracer::~RayTracer()
{
  delete mShader;
}


bool RayTracer::initGL(QObject *qParent)
{
  if(!mInitialized)
    {
      initializeOpenGLFunctions();
      
      // load shaders
      mQuadShader = new Shader();
      if(!mQuadShader->loadProgram("./shaders/rayQuad.vsh", "./shaders/rayQuad.fsh",
                               {"posAttr", "texCoordAttr"},
                               {"fogStart", "fogEnd", "dirScale", "rayTex"} ))
        {
          LOGE("Block quad shader failed to load!");
          delete mQuadShader;
          mQuadShader = nullptr;
          return false;
        }
      else
        {
          mQuadShader->bind();
          mQuadShader->setUniform("fogStart", mFogStart);
          mQuadShader->setUniform("fogEnd", mFogEnd);
          mQuadShader->setUniform("dirScale", mDirScale);
          mQuadShader->setUniform("rayTex", 1);
          mQuadShader->release();
        }

      mShader = new ComputeShader();
      if(!mShader->loadProgram("./shaders/ray.csh", {"screenSize", "camPos",
                                                     "v00", "v10", "v01", "v11", "blockTex"} ))
        {
          LOGE("Block ray trace shader failed to load!");
          delete mShader;
          mShader = nullptr;
          delete mQuadShader;
          mQuadShader = nullptr;
          return false;
        }
      else
        {
          mShader->bind();
          mShader->setUniform("blockTex", 0);
          mShader->setUniform("screenSize", mScreenSize);
          mShader->setUniform("aspect", 1.0f);
          mShader->setUniform("vEye", Vector3f{1,0,0});
          mShader->setUniform("vRight", Vector3f{0,-1,0});
          mShader->setUniform("vUp", Vector3f{0,0,1});
          setupCompute();
          mShader->release();
        }

      mQuad = new QuadBuffer();
      if(!mQuad->initGL(mQuadShader))
        {
          LOGE("Simple block shader failed to load!");
          delete mShader;
          mShader = nullptr;
          delete mQuad;
          mQuad = nullptr;
          return false;
        }
      
      // load block textures
      mTexAtlas = new cTextureAtlas(qParent, ATLAS_BLOCK_SIZE);
      if(!mTexAtlas->create("./res/texAtlas.png"))
        {
          LOGE("Failed to load texture atlas!");
          mQuad->cleanupGL();
          delete mQuad;
          mQuad = nullptr;
          delete mShader;
          mShader = nullptr;
          delete mTexAtlas;
          mTexAtlas = nullptr;
          return false;
        }
      
      mInitialized = true;
    }
  return true;
}

void RayTracer::cleanupGL()
{
  if(mInitialized)
    {
      mQuad->cleanupGL();
      delete mQuad;
      delete mShader;
      mTexAtlas->destroy();
      delete mTexAtlas;
      mQuad = nullptr;
      mShader = nullptr;
      mTexAtlas = nullptr;
      mInitialized = false;
    }
}

static bool init = false;
void RayTracer::render(const Matrix4 &pvm, const Point3f &camPos)
{ 
  mShader->waitForFinish();
  mQuadShader->bind();
  {
    std::lock_guard<std::mutex> lock(mRenderLock);
    if(mFogChanged)
      {
        mQuadShader->setUniform("fogStart", mFogStart);
        mQuadShader->setUniform("fogEnd", mFogEnd);
        mQuadShader->setUniform("dirScale", mDirScale);
        mFogChanged = false;
      }
    mQuad->render();
  }
  mQuadShader->release();
  glFinish();
  
  mShader->bind();
  mShader->setUniform("camPos", Chunk::blockPos(camPos));
  mShader->setUniform("screenSize", mScreenSize);
  const float fov = mCamera->getFovY();
  const Vector3f eye = mCamera->getEye();
  const Vector3f right = mCamera->getRight() * tan(fov/2.0f)*mCamera->getAspect();
  const Vector3f up = crossProduct(right, eye).normalized() * tan(fov/2.0f);
  
  mShader->setUniform("v00", (eye - right - up).normalized());
  mShader->setUniform("v10", (eye + right - up).normalized());
  mShader->setUniform("v01", (eye - right + up).normalized());
  mShader->setUniform("v11", (eye + right + up).normalized());
  
  mTexAtlas->bind();
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBlockBuffer);
  if(!init && prepareChunkData(Hash::hash(mCenter)))
    {
      init = true;
    }
  mShader->dispatch(Point3i{mScreenSize[0], mScreenSize[1], 1});
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  mTexAtlas->release();
  mShader->release();
}

void RayTracer::setCenter(const Point3i &center)
{
  mCenter = center;
  init = false;
}

void RayTracer::load(hash_t hash, Chunk *chunk)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  mChunks.emplace(hash, chunk);
  if(hash == 0 )
    { init = false; }
}
void RayTracer::unload(hash_t hash)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  mChunks.erase(hash);
}

void RayTracer::setFog(float fogStart, float fogEnd, const Vector3f &dirScale)
{
  std::lock_guard<std::mutex> lock(mRenderLock);
  mFogStart = fogStart;
  mFogEnd = fogEnd;
  mDirScale = dirScale;
  mFogChanged = true;
}

void RayTracer::setCamera(Camera *camera)
{ mCamera = camera; }
void RayTracer::setFrustumCulling(bool on)
{ mFrustumCulling = on; }

void RayTracer::pauseFrustum()
{
  std::lock_guard<std::mutex> lock(mRenderLock);
  //Matrix4 pvm = mFrustum->getProjection() * mFrustum->getView();
  if(mFrustumPaused)
    {
      mFrustumRender.clear();
      int num = 0;
      for(auto &iter : mChunks)
        {
          if(mCamera->cubeInFrustum(Hash::unhash(iter.first)*Chunk::size, Chunk::size))
            { mFrustumRender.insert(iter.first); num++; }
          else
            { mFrustumRender.erase(iter.first); }
        }
      mFrustumPaused = false;
    }
  else
    {
      mFrustumPaused = true;
    }
}


void RayTracer::setScreenSize(const Point2i &size)
{
  if(size != mScreenSize)
    {
      mScreenSize = size;
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, mComputeTex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mScreenSize[0], mScreenSize[1],
                   0, GL_RGBA, GL_FLOAT, NULL );
    }
}

bool RayTracer::prepareChunkData(hash_t hash)
{
  Chunk *chunk;
  auto cIter = mChunks.find(hash);
  if(cIter != mChunks.end())
    {
      chunk = cIter->second;
    }
  else
    { return false; }

  GLuint bid;
  auto bufIter = mChunkBuffers.find(hash);
  if(bufIter == mChunkBuffers.end())
    {
      glGenBuffers(1, &bid);
      mChunkBuffers.emplace(hash, bid);
      mChunkData.emplace(hash, std::vector<int>(Chunk::totalSize));
    }
  else
    { bid = bufIter->second; }

  auto iter = mChunkData.find(hash);
  iter->second.clear();

  for(auto &b : chunk->data())
    { iter->second.push_back((int)b-1); }
  
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, bid);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int)*Chunk::totalSize,
               iter->second.data(), GL_STATIC_DRAW );
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, bid);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  //mShader->setUniform("numBlocks", (int)iter->second.size());
  return true;
}

void RayTracer::setupCompute()
{
  // create output texture
  glGenTextures(1, &mComputeTex);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, mComputeTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mScreenSize[0], mScreenSize[1],
               0, GL_RGBA, GL_FLOAT, NULL );
  glBindImageTexture(1, mComputeTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
}
