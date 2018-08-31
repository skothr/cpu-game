#include "chunkMesh.hpp"

#include "chunk.hpp"
#include "shader.hpp"
#include "meshBuffer.hpp"
#include <unistd.h>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
//#include <qopengl.h>

ChunkMesh::ChunkMesh(bool doubleBuffered)
  : mDB(doubleBuffered)
{

}

ChunkMesh::~ChunkMesh()
{
  
}

bool ChunkMesh::initialized() const
{ return mLoaded; }

// make sure to call this from the OpenGL thread!
bool ChunkMesh::initGL(Shader *shader)
{
  if(!mLoaded)
    {
      initializeOpenGLFunctions();
      
      mActiveBuffer = 0;
      mBuffers[0] = new cMeshBuffer();
      mBuffers[1] = new cMeshBuffer();
      activeBuffer()->initGL(shader);
      inactiveBuffer()->initGL(shader);
      mLoaded = true;
      return true;
    }
  else
    { return false; }
}

void ChunkMesh::cleanupGL()
{
  mBuffers[0]->cleanupGL();
  mBuffers[1]->cleanupGL();
  delete mBuffers[0];
  delete mBuffers[1];
}


void ChunkMesh::uploadData(const MeshData &data)
{
  //LOGD("MESH UPLOADING --> %di, %dv", data.indices.size(), data.vertices.size());
  inactiveBuffer()->uploadData(data);
  //LOGD("SWAPPING");
  swapBuffers();
  //LOGD("DONE");
}

void ChunkMesh::render()
{
  //LOGD("CHUNK MESH RENDERING --> %d, %d", (long)activeVBO(), (long)activeIBO());
  //if(mLoaded)
  {
    activeBuffer()->render();
  }
}

void ChunkMesh::detachData()
{
  //activeBuffer()->detachData();
  //inactiveBuffer()->detachData();
}

/*

void ChunkMesh::startUpdating(cSimpleVertex* &verticesOut, unsigned int* &indicesOut, int maxSize)
{
  LOGD("CHUNK MESH MAPPING");
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
}
void ChunkMesh::finishUpdating(int numIndices, std::vector<cSimpleVertex> &vData, std::vector<unsigned int> &iData)
{
  //LOGD("UPDATING BUFFER %d", mActiveBuffer);
  inactiveBuffer()->finishUpdating(numIndices, vData, iData);
  //glFinish();
  swapBuffers();
}
*/
