#include "mesh.hpp"


cMesh::cMesh()
  : mName("")
{

}

cMesh::cMesh(const objl::Mesh &mesh)
  : mName(mesh.MeshName), mMaterial(mesh.MeshMaterial)
{
  setMesh(mesh.Vertices, mesh.Indices);
}

cMesh::~cMesh()
{
  
}

void cMesh::setMode(GLenum mode)
{
  mMode = mode;
}

bool cMesh::setMesh(const std::vector<objl::Vertex> &vertices,
		    const std::vector<unsigned int> &indices )
{
  mVertices = vertices;
  mIndices = indices;
  mNeedUpdate |= UPDATE_VERTICES | UPDATE_INDICES;
  return true;
}
bool cMesh::setVertices(const std::vector<objl::Vertex> &vertices)
{
  mVertices = vertices;
  mNeedUpdate |= UPDATE_VERTICES;
  return true;
}
bool cMesh::setIndices(const std::vector<unsigned int> &indices)
{
  mIndices = indices;
  mNeedUpdate |= UPDATE_INDICES;
  return true;
}

void cMesh::cleanupGL()
{
  mVao->destroy();
  delete mVao;
  mVbo->destroy();
  delete mVbo;
  mIbo->destroy();
  delete mIbo;
}

// make sure to call this from the OpenGL thread!
bool cMesh::initGL(cShader *shader)
{
  mVbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  if(!mVbo->create())
    {
      LOGE("VBO create failed in cMesh!!");
      return false;
    }
  mVbo->bind();
  mVbo->setUsagePattern(QOpenGLBuffer::StaticDraw);

  mIbo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
  if(!mIbo->create())
    {
      LOGE("IBO create failed in cMesh!!");
      return false;
    }
  mIbo->bind();
  mIbo->setUsagePattern(QOpenGLBuffer::StaticDraw);

  if(mVertices.size() > 0 && mIndices.size() > 0)
    {
      if(mNeedUpdate & UPDATE_VERTICES)
	{
	  mVbo->allocate(mVertices.data(), mVertices.size()*sizeof(objl::Vertex));
	  mNeedUpdate &= ~UPDATE_VERTICES;
	}
      if(mNeedUpdate & UPDATE_INDICES)
	{
	  mIbo->allocate(mIndices.data(), mIndices.size()*sizeof(unsigned int));
	  mNeedUpdate &= ~UPDATE_INDICES;
	}
    }
  mVao = new QOpenGLVertexArrayObject();
  if(!mVao->create())
    {
      LOGE("VAO create failed in cMesh!!");
      return false;
    }
  mVao->bind();

  // configure attributes
  shader->setAttrBuffer(0, GL_FLOAT, 0, 3, sizeof(cVertex) );
  shader->setAttrBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, sizeof(cVertex) );
  shader->setAttrBuffer(2, GL_FLOAT, 6 * sizeof(float), 2, sizeof(cVertex) );
  
  mVao->release();
  mVbo->release();
  mIbo->release();

  LOGD("done");
  return true;
}

void cMesh::render(cShader *shader)
{
  if(mVertices.size() > 0 && mIndices.size() > 0)
    {
      //LOGD("binding...");
      mVao->bind();
      mVbo->bind();
      mIbo->bind();
      if(mNeedUpdate & UPDATE_VERTICES)
	{
	  LOGD("Updating vertices...");
	  mVbo->allocate(mVertices.data(), mVertices.size()*sizeof(objl::Vertex));
	  mNeedUpdate &= ~UPDATE_VERTICES;
	  LOGD("NEED UPDATE: 0x%02X", mNeedUpdate);
	}
      if(mNeedUpdate & UPDATE_INDICES)
	{
	  LOGD("Updating indices...");
	  mIbo->allocate(mIndices.data(), mIndices.size()*sizeof(unsigned int));
	  mNeedUpdate &= ~UPDATE_INDICES;
	  LOGD("NEED UPDATE: 0x%02X", mNeedUpdate);
	}
      glDrawElements(mMode, mIndices.size(), GL_UNSIGNED_INT, 0);
      mIbo->release();
      mVbo->release();
      mVao->release();
    }
}
