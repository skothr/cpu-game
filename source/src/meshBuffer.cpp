#include "meshBuffer.hpp"

#include "chunk.hpp"
#include "shader.hpp"
#include <QOpenGLBuffer>


cMeshBuffer::cMeshBuffer()
{

}

cMeshBuffer::~cMeshBuffer()
{
  
}

bool cMeshBuffer::initialized() const
{ return mLoaded; }

// make sure to call this from the OpenGL thread!
bool cMeshBuffer::initGL(cShader *shader, const std::vector<cSimpleVertex> *vert,
                         const std::vector<unsigned int> *ind )
{
  if(!mLoaded)
    {
      initializeOpenGLFunctions();
      
      mVbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
      if(!mVbo->create())
        {
          LOGE("VBO create failed in cMeshBuffer!!");
          return false;
        }
      mVbo->setUsagePattern(QOpenGLBuffer::DynamicDraw);

      mIbo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
      if(!mIbo->create())
        {
          LOGE("IBO create failed in cMeshBuffer!!");
          return false;
        }
      mIbo->setUsagePattern(QOpenGLBuffer::DynamicDraw);

      // VAO
      mVao = new QOpenGLVertexArrayObject();
      if(!mVao->create())
	{
	  LOGE("VAO create failed in cMeshBuffer!!");
	  return false;
	}
      mVao->bind();     
      mIbo->bind();
      mVbo->bind();
      shader->setAttrBuffer(0, GL_FLOAT, 0, 3, sizeof(cSimpleVertex) );
      shader->setAttrBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, sizeof(cSimpleVertex));
      shader->setAttrBuffer(2, GL_FLOAT, 6 * sizeof(float), 3, sizeof(cSimpleVertex));
      shader->setAttrBuffer(3, GL_FLOAT, 9 * sizeof(float), 1, sizeof(cSimpleVertex));

      if(vert && ind)
        {
          mVbo->allocate(vert->data(), sizeof(cSimpleVertex)*vert->size());
          mIbo->allocate(ind->data(), sizeof(unsigned int)*ind->size());
          mNumDraw = ind->size();
        }
      
      mVbo->release();
      mIbo->release();
      mVao->release();

      mLoaded = true;
      return true;
    }
  else
    { return false; }
}

void cMeshBuffer::cleanupGL()
{
  mVao->destroy();
  mVbo->destroy();
  mIbo->destroy();
  delete mVbo;
  delete mIbo;
  delete mVao;
  mLoaded = false;
}

void cMeshBuffer::render()
{
  //LOGD("CHUNK MESH RENDERING --> %d, %d", (long)activeVBO(), (long)activeIBO());
  if(mVbo && mIbo && mVao && mNumDraw > 0)
  {
    mVao->bind();
    //mIbo->bind();
    //mVbo->bind();
    glDrawElements(GL_TRIANGLES, mNumDraw, GL_UNSIGNED_INT, 0);
    //mVbo->release();
    //mIbo->release();
    mVao->release();

    //glFinish();
  }
}





void cMeshBuffer::startUpdating(cSimpleVertex* &verticesOut, unsigned int* &indicesOut, int maxSize)
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
void cMeshBuffer::finishUpdating(int numIndices, std::vector<cSimpleVertex> &vData, std::vector<unsigned int> &iData)
{
  
  //std::lock_guard<std::mutex> lock(mBufferLock);
  //inactiveVBO()->bind();
  //inactiveVBO()->unmap();
  //inactiveVBO()->release();
      
  //inactiveIBO()->bind();
  //inactiveIBO()->unmap();
  //inactiveIBO()->release();
  
  mVao->bind();
  mVbo->bind();
  mIbo->bind();
  mVbo->allocate(vData.data(), sizeof(cSimpleVertex)*vData.size());
  mIbo->allocate(iData.data(), sizeof(unsigned int)*iData.size());
  
  //verticesOut = (cSimpleVertex*)inactiveVBO()->map(QOpenGLBuffer::WriteOnly);
  //  indicesOut = (unsigned int*)inactiveIBO()->map(QOpenGLBuffer::WriteOnly);
  //glFinish();

  mVao->release();
  mVbo->release();
  mIbo->release();
  
  mNumDraw = numIndices;
  //glFinish();
}
