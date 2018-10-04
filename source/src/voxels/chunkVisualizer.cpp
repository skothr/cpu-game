#include "chunkVisualizer.hpp"

#include "blockSides.hpp"
#include "shader.hpp"
#include "chunkMap.hpp"
#include "vertex.hpp"
#include "meshBuffer.hpp"

#include <QMatrix4x4>
#include <array>
#include <unordered_map>



// CUBE FACE INDICES
const std::array<unsigned int, 6> faceIndices =
  { 0, 1, 2, 3, 1, 0 };
const std::array<unsigned int, 6> reverseIndices =
  { 0, 1, 3, 2, 1, 0 };
const std::array<unsigned int, 6> flippedIndices =
  { 0, 3, 2, 1, 2, 3 };
std::unordered_map<blockSide_t, std::array<cSimpleVertex, 4>> faceVertices =
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

const std::array<blockSide_t, 6> meshSides =
  { blockSide_t::PX, blockSide_t::PY,
    blockSide_t::PZ, blockSide_t::NX,
    blockSide_t::NY, blockSide_t::NZ };
const std::array<Point3i, 6> meshSideDirections =
  { Point3i{1,0,0}, Point3i{0,1,0}, Point3i{0,0,1},
    Point3i{-1,0,0}, Point3i{0,-1,0}, Point3i{0,0,-1} };


ChunkVisualizer::ChunkVisualizer(const Vector2i &size)
  : mSize(size)
{
  
}

ChunkVisualizer::~ChunkVisualizer()
{
  
}

bool ChunkVisualizer::initGL(QObject *qParent)
{
  if(!mInitialized)
    {
      initializeOpenGLFunctions();
      
      mShader = new Shader(qParent);
      if(!mShader->loadProgram("./shaders/chunkVisualizer.vsh", "./shaders/chunkVisualizer.fsh",
                               {"posAttr", "normalAttr", "texCoordAttr", "lightAttr"},
                               {"pvm"} ))
        {
          LOGE("Chunk visualizer shader failed to load!");
          delete mShader;
          mShader = nullptr;
          return false;
        }
      mShader->setUniform("pvm", Matrix4(QMatrix4x4()));
      
      mMesh = new cMeshBuffer();
      mMesh->initGL(mShader);
      mInitialized = true;
    }
  return true;
}

void ChunkVisualizer::cleanupGL()
{
  if(mInitialized)
    {
      mMesh->cleanupGL();
      delete mMesh;
      delete mShader;
      mInitialized = false;
    }
}

#define VP_PADDING 20

void ChunkVisualizer::render()
{
  std::lock_guard<std::mutex> lock(mLock);
  if(mNewData)
    { // upload new data
      mMesh->uploadData(mMeshData);
      mNewData = false;
    }
  
  // calc p/v matrices
  QMatrix4x4 miniMapP;
  miniMapP.setToIdentity();
  miniMapP.perspective(50, 1.0, 0.01, 1000.0);

  Point3f cPos = mCenter;
  Point3f cRad = mRadius;
  Vector3f eye{0,-1,0};
  Vector3f up{0,0,1};
  QMatrix4x4 miniMapV;
  miniMapV.setToIdentity();
  miniMapV.lookAt(toQt(eye*45.0),
                  toQt(Vector3f{0,0,0}),
                  toQt(up) );
  
  QMatrix4x4 miniMapT;
  miniMapT.setToIdentity();
  miniMapT.translate(toQt(-mCenter));
  miniMapT.rotate(mEyeAngle[0]*100, QVector3D(0,0,1));
  miniMapT.rotate(mEyeAngle[1]*100, QVector3D(1,0,0));
  
  // save current viewport/clear color
  GLint vp[4];
  //GLfloat cc[3];
  glGetIntegerv(GL_VIEWPORT, vp);
  //glGetFloatv(GL_COLOR_CLEAR_VALUE, cc);
  
  // set viewport/clear color for visualizer
  glViewport(VP_PADDING, VP_PADDING, mSize[0] - 2*VP_PADDING, mSize[1] - 2*VP_PADDING);
  //glClearColor(0.1,0.1,0.1, 1.0);

  // render
  mShader->bind();
  mShader->setUniform("pvm", Matrix4(miniMapP*miniMapV*miniMapT));
  glEnable(GL_SCISSOR_TEST);
  glScissor(VP_PADDING, VP_PADDING, mSize[0] - 2*VP_PADDING, mSize[1] - 2*VP_PADDING);
  //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);
  mMesh->render();
  mShader->release();

  // restore old viewport/clear color
  glViewport(vp[0], vp[1], vp[2], vp[3]);
  //glClearColor(cc[0], cc[1], cc[2], 1.0);
}

void ChunkVisualizer::setSize(const Vector2i &size)
{
  mSize = size;
}
void ChunkVisualizer::setCenter(const Point3i &center)
{
  mCenter = center;
}
void ChunkVisualizer::setRadius(const Vector3i &radius)
{
  mRadius = radius;
}
void ChunkVisualizer::setEyeAngle(const Vector3f &eyeAngle)
{
  mEyeAngle = eyeAngle;
}
Vector3f ChunkVisualizer::getEyeAngle() const
{
  return mEyeAngle;
}


void ChunkVisualizer::setState(hash_t hash, ChunkState state)
{
  std::lock_guard<std::mutex> lock(mLock);
  auto iter = mChunks.find(hash);
  if(iter == mChunks.end())
    {
      mChunks.emplace(hash, state);
      updateMesh();
    }
  else
    {
      ChunkState old = iter->second;
      iter->second = state;
      if(state != old)
        { updateMesh(); }
    }
}
void ChunkVisualizer::unload(hash_t hash)
{
  std::lock_guard<std::mutex> lock(mLock);
  auto iter = mChunks.find(hash);
  if(iter != mChunks.end())
    {
      mChunks.erase(hash);
      updateMesh();
    }
}

void ChunkVisualizer::updateMesh()
{
  mMeshData.vertices().clear();
  mMeshData.indices().clear();
  for(auto iter : mChunks)
    {
      Point3f cp = Hash::unhash(iter.first);
      Vector3f col = chunkStateColor(iter.second);
      for(auto &f : faceVertices)
        {
          const unsigned int numVert = mMeshData.vertices().size();
          for(auto &v : f.second)
            {
              mMeshData.vertices().emplace_back(cp + v.pos*0.1 - (Vector3f{1,1,1} - v.pos)*0.1,
                                                col, v.texcoord,
                                                1, 5.0f );
            }
          for(auto i : faceIndices)
            { mMeshData.indices().push_back(numVert + i); }
        }
    }

  const int v = mMeshData.vertices().size();
  mMeshData.vertices().emplace_back(mCenter + Vector3f{-1,0,2*mRadius[2]},
                                    Vector3f{1,0,0}, Vector2f{0,0}, 1, 1);
  mMeshData.vertices().emplace_back(mCenter + Vector3f{0,0,2*mRadius[2]+2},
                                    Vector3f{1,0,0}, Vector2f{0,0}, 1, 1);
  mMeshData.vertices().emplace_back(mCenter + Vector3f{1,0,2*mRadius[2]},
                                    Vector3f{1,0,0}, Vector2f{0,0}, 1, 1);

  mMeshData.indices().push_back(v+0);
  mMeshData.indices().push_back(v+1);
  mMeshData.indices().push_back(v+2);
  mMeshData.indices().push_back(v+2);
  mMeshData.indices().push_back(v+1);
  mMeshData.indices().push_back(v+0);


  // mMeshData.vertices().emplace_back(Point3f{-1,-1,0}, Vector3f{1,1,1}, Vector2f{1,1}, 1, 5.0f);
  // mMeshData.vertices().emplace_back(Point3f{1,-1,0}, Vector3f{1,1,1}, Vector2f{1,1}, 1, 5.0f);
  // mMeshData.vertices().emplace_back(Point3f{-1,1,0}, Vector3f{1,1,1}, Vector2f{1,1}, 1, 5.0f);
  // mMeshData.vertices().emplace_back(Point3f{1,1,0}, Vector3f{1,1,1}, Vector2f{1,1}, 1, 5.0f);
  // mMeshData.indices().push_back(0);
  // mMeshData.indices().push_back(1);
  // mMeshData.indices().push_back(2);
  // mMeshData.indices().push_back(2);
  // mMeshData.indices().push_back(1);
  // mMeshData.indices().push_back(3);
  mNewData = true;
}
