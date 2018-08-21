#ifndef CHUNK_MESH_HPP
#define CHUNK_MESH_HPP

#include "vertex.hpp"
#include "logging.hpp"
#include <vector>
#include <mutex>

#include <QOpenGLFunctions_4_3_Core>

class QOpenGLBuffer;
class QOpenGLVertexArrayObject;
class cShader;
class cMeshBuffer;

class cChunkMesh : protected QOpenGLFunctions_4_3_Core
{
public:
  cChunkMesh(bool doubleBuffered = true);
  ~cChunkMesh();

  bool initialized() const;
  
  bool initGL(cShader *shader, const std::vector<cSimpleVertex> *vert = nullptr,
              const std::vector<unsigned int> *ind = nullptr );
  void cleanupGL();
  void render();

  void startUpdating(cSimpleVertex* &verticesOut, unsigned int* &indicesOut, int maxSize);
  void finishUpdating(int numIndices, std::vector<cSimpleVertex> &vData, std::vector<unsigned int> &iData);

  
private:
  //std::vector<cSimpleVertex> mVertices;
  //std::vector<unsigned int> mIndices;

  void detachData();

  cMeshBuffer* activeBuffer() { return mBuffers[mActiveBuffer]; }
  cMeshBuffer* inactiveBuffer() { return mBuffers[(mActiveBuffer+1)%2]; }

  /*
  QOpenGLBuffer* activeVBO() { return mVbo[mActiveBuffer]; }
  QOpenGLBuffer* activeIBO() { return mIbo[mActiveBuffer]; }
  QOpenGLVertexArrayObject* activeVAO() { return mVao[mActiveBuffer]; }
  QOpenGLBuffer* inactiveVBO() { return mVbo[(mActiveBuffer+1)%2]; }
  QOpenGLBuffer* inactiveIBO() { return mIbo[(mActiveBuffer+1)%2]; }
  QOpenGLVertexArrayObject* inactiveVAO() { return mVao[(mActiveBuffer+1)%2]; }
  int activeMaxSize() const { return mMaxVertices[mActiveBuffer]; }
  int inactiveMaxSize() const { return mMaxVertices[(mActiveBuffer+1)%2]; }
  int activeDraw() const { return mNumDraw[mActiveBuffer]; }
  int inactiveDraw() const { return mNumDraw[(mActiveBuffer+1)%2]; }
  int& activeMaxSize() { return mMaxVertices[mActiveBuffer]; }
  int& inactiveMaxSize() { return mMaxVertices[(mActiveBuffer+1)%2]; }
  int& activeDraw() { return mNumDraw[mActiveBuffer]; }
  int& inactiveDraw() { return mNumDraw[(mActiveBuffer+1)%2]; }
  */
  void swapBuffers()
  {
    //LOGD("CHUNK MESH SWAPPING");
    //std::lock_guard<std::mutex> lock(mBufferLock);
    mActiveBuffer = (mActiveBuffer + 1) % 2;
  }
  
  std::mutex mBufferLock;
  bool mLoaded = false;
  bool mDB;

  int mActiveBuffer = 0;
  cMeshBuffer *mBuffers[2] = {nullptr, nullptr};
};

#endif // CHUNK_MESH_HPP
