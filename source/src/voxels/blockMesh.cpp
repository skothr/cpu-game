#include "blockMesh.hpp"



BlockMesh::BlockMesh()
{

}

BlockMesh::BlockMesh(const std::vector<cSimpleVertex> &vertices)
  : mVertices(vertices)
{ }

BlockMesh::~BlockMesh()
{
  
}


bool BlockMesh::setMesh(const std::vector<cSimpleVertex> &vertices)
{
  mVertices = vertices;
  mNeedUpdate = true;
  return true;
}

void BlockMesh::cleanupGL()
{
  mVao->destroy();
  delete mVao;
  mVbo->destroy();
  delete mVbo;
}

// make sure to call this from the OpenGL thread!
bool BlockMesh::initGL(Shader *shader)
{
  mVbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  if(!mVbo->create())
    {
      LOGE("VBO create failed in BlockMesh!!");
      return false;
    }
  mVbo->bind();
  mVbo->setUsagePattern(QOpenGLBuffer::StaticDraw);

  if(mVertices.size() > 0)
    {
      if(mNeedUpdate)
	{
	  mVbo->allocate(mVertices.data(), mVertices.size()*sizeof(cSimpleVertex));
	  mNeedUpdate = false;
	}
    }
  mVao = new QOpenGLVertexArrayObject();
  if(!mVao->create())
    {
      LOGE("VAO create failed in BlockMesh!!");
      return false;
    }
  mVao->bind();

  // configure attributes
  shader->setAttrBuffer(0, GL_FLOAT, 0,                 3, sizeof(cSimpleVertex) );
  shader->setAttrBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, sizeof(cSimpleVertex) );
  shader->setAttrBuffer(2, GL_FLOAT, 6 * sizeof(float), 3, sizeof(cSimpleVertex) );
  
  mVao->release();
  mVbo->release();

  LOGD("done");
  return true;
}

void BlockMesh::render(Shader *shader)
{
  if(mVertices.size() > 0)
    {
      mVao->bind();
      mVbo->bind();
      if(mNeedUpdate && !mUpdating)
	{
	  mVbo->allocate(mVertices.data(), mVertices.size()*sizeof(cSimpleVertex));
	  mNeedUpdate = false;
	}
      glDrawArrays(GL_TRIANGLES, 0, mVertices.size());
      mVbo->release();
      mVao->release();
    }
}
