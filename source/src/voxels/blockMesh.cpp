#include "blockMesh.hpp"



cBlockMesh::cBlockMesh()
{

}

cBlockMesh::cBlockMesh(const std::vector<cSimpleVertex> &vertices)
  : mVertices(vertices)
{ }

cBlockMesh::~cBlockMesh()
{
  
}


bool cBlockMesh::setMesh(const std::vector<cSimpleVertex> &vertices)
{
  mVertices = vertices;
  mNeedUpdate = true;
  return true;
}

void cBlockMesh::cleanupGL()
{
  mVao->destroy();
  delete mVao;
  mVbo->destroy();
  delete mVbo;
}

// make sure to call this from the OpenGL thread!
bool cBlockMesh::initGL(cShader *shader)
{
  mVbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  if(!mVbo->create())
    {
      LOGE("VBO create failed in cBlockMesh!!");
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
      LOGE("VAO create failed in cBlockMesh!!");
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

void cBlockMesh::render(cShader *shader)
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
