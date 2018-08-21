#include "chunkMesh.hpp"

#include "chunk.hpp"
#include "shader.hpp"
#include "meshBuffer.hpp"
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <unistd.h>

cChunkMesh::cChunkMesh(bool doubleBuffered)
  : mDB(doubleBuffered)
{

}

cChunkMesh::~cChunkMesh()
{
  
}

bool cChunkMesh::initialized() const
{ return mLoaded; }

// make sure to call this from the OpenGL thread!
bool cChunkMesh::initGL(cShader *shader, const std::vector<cSimpleVertex> *vert,
                        const std::vector<unsigned int> *ind )
{
  if(!mLoaded)
    {
      initializeOpenGLFunctions();
      
      mBuffers[0] = new cMeshBuffer();
      mBuffers[1] = new cMeshBuffer();
      mActiveBuffer = 0;
      activeBuffer()->initGL(shader, vert, ind);
      inactiveBuffer()->initGL(shader);

      mLoaded = true;
      return true;
    }
  else
    { return false; }
}

void cChunkMesh::cleanupGL()
{
  mBuffers[0]->cleanupGL();
  mBuffers[1]->cleanupGL();
  delete mBuffers[0];
  delete mBuffers[1];
}

void cChunkMesh::detachData()
{
  /*
  //GLuint vboId = mVbo->bufferId();
  //GLuint iboId = mIbo->bufferId();
  mVbo->bind();
  glBufferData(GL_VERTEX_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
  mIbo->bind();
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);

  mVbo->release();
  mIbo->release();
  */
}


void cChunkMesh::startUpdating(cSimpleVertex* &verticesOut, unsigned int* &indicesOut, int maxSize)
{
  LOGD("CHUNK MESH MAPPING");
  /*
  if(maxSize <= inactiveMaxSize())
    {
      inactiveVBO()->bind();
      inactiveIBO()->bind();
      verticesOut = (cSimpleVertex*)inactiveVBO()->map(QOpenGLBuffer::WriteOnly);
      
      indicesOut = (unsigned int*)inactiveIBO()->map(QOpenGLBuffer::WriteOnly);
      inactiveIBO()->release();
      inactiveVBO()->release();
      
      inactiveMaxSize() = maxSize;
    }
  else
    {
      //std::lock_guard<std::mutex> lock(mBufferLock);
      inactiveVBO()->bind();
      inactiveVBO()->allocate(sizeof(cSimpleVertex)*maxSize);
      verticesOut = (cSimpleVertex*)inactiveVBO()->map(QOpenGLBuffer::WriteOnly);
      // inactiveVBO()->release();
      
      inactiveIBO()->bind();
      inactiveIBO()->allocate(sizeof(unsigned int)*maxSize);
      indicesOut = (unsigned int*)inactiveIBO()->map(QOpenGLBuffer::WriteOnly);
      // inactiveIBO()->release();
      
    }
  */
}
void cChunkMesh::finishUpdating(int numIndices, std::vector<cSimpleVertex> &vData, std::vector<unsigned int> &iData)
{
  //LOGD("UPDATING BUFFER %d", mActiveBuffer);
  inactiveBuffer()->finishUpdating(numIndices, vData, iData);
  //glFinish();
  swapBuffers();
}

void cChunkMesh::render()
{
  //LOGD("CHUNK MESH RENDERING --> %d, %d", (long)activeVBO(), (long)activeIBO());
  if(activeBuffer() && activeBuffer()->numDraw() > 0)
  {
    //LOGD("RENDERING BUFFER %dv", activeBuffer()->numDraw());
    //activeVBO()->bind();
    //activeIBO()->bind();
    activeBuffer()->render();
    //glFinish();
  }
}
