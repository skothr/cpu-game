#include "mesh.hpp"

#include "chunk.hpp"

Mesh::Mesh()//bool doubleBuffered)
  : mName("")//, mRenderLock(new std::mutex()), mDB(doubleBuffered)
{

}

Mesh::Mesh(const objl::Mesh &mesh)//, bool doubleBuffered)
  : mName(mesh.MeshName)//, mMaterial(mesh.MeshMaterial)
    //mRenderLock(new std::mutex()), mDB(doubleBuffered)
{
  setMesh(mesh.Vertices,
	  std::vector<unsigned int>(mesh.Indices.begin(), mesh.Indices.end() ));
}

Mesh::~Mesh()
{
  //delete mRenderLock;
}

void Mesh::setMode(GLenum mode)
{
  mMode = mode;
}

bool Mesh::setMesh(const std::vector<objl::Vertex> &vertices,
		    const std::vector<unsigned int> &indices )
{
  mVertices.clear();
  mVertices.reserve(vertices.size());
  for(auto &v : vertices)
    {
      mVertices.push_back({{v.Position.X, v.Position.Y, v.Position.Z},
				     {v.Normal.X, v.Normal.Y, v.Normal.Z},
				     {v.TextureCoordinate.X, v.TextureCoordinate.Y }});
    }

  mIndices = indices;
  //std::lock_guard<std::mutex> lock(*mRenderLock);
  mNeedUpdate |= UPDATE_VERTICES | UPDATE_INDICES;
  //mCurrVbo = (mCurrVbo + 1) % 2;
  return true;
}
bool Mesh::setMesh(const std::vector<cTexVertex> &vertices,
		    const std::vector<unsigned int> &indices )
{
  mVertices = vertices;
  mIndices = indices;
  //std::lock_guard<std::mutex> lock(*mRenderLock);
  mNeedUpdate |= UPDATE_VERTICES | UPDATE_INDICES;
  //mCurrVbo = (mCurrVbo + 1) % 2;
  return true;
}
bool Mesh::setVertices(const std::vector<cTexVertex> &vertices)
{
  mVertices = vertices;
  //std::lock_guard<std::mutex> lock(*mRenderLock);
  mNeedUpdate |= UPDATE_VERTICES;
  // mCurrVbo = (mCurrVbo + 1) % 2;
  return true;
}
bool Mesh::setIndices(const std::vector<unsigned int> &indices)
{
  mIndices = indices;
  //std::lock_guard<std::mutex> lock(*mRenderLock);
  mNeedUpdate |= UPDATE_INDICES;
  //mCurrVbo = (mCurrVbo + 1) % 2;
  return true;
}

void Mesh::cleanupGL()
{
  mVao->destroy();
  delete mVao;
  /*
  for(int i = 0; i < 2; i++)
    {
      mVbo[i]->destroy();
      delete mVbo[i];
      mIbo[i]->destroy();
      delete mIbo[i];
    }
  */
  mVbo->destroy();
  delete mVbo;
  mIbo->destroy();
  delete mIbo;
}

// make sure to call this from the OpenGL thread!
bool Mesh::initGL(Shader *shader)
{
  if(!mLoaded)
    {
      initializeOpenGLFunctions();
      //for(int i = 0; i < 2; i++)
      //{
	  mVbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
	  if(!mVbo->create())
	    {
	      LOGE("VBO create failed in Mesh!!");
	      return false;
	    }
	  mVbo->setUsagePattern(QOpenGLBuffer::DynamicDraw);

	  mIbo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
	  if(!mIbo->create())
	    {
	      LOGE("IBO create failed in Mesh!!");
	      return false;
	    }
	  mIbo->setUsagePattern(QOpenGLBuffer::DynamicDraw);
          /*
	  mVbo2 = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
	  if(!mVbo2->create())
	    {
	      LOGE("VBO create failed in Mesh!!");
	      return false;
	    }
	  mVbo2->bind();
	  mVbo2->setUsagePattern(QOpenGLBuffer::DynamicDraw);

	  mIbo2 = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
	  if(!mIbo2->create())
	    {
	      LOGE("IBO create failed in Mesh!!");
	      return false;
	    }
	  mIbo2->bind();
	  mIbo2->setUsagePattern(QOpenGLBuffer::DynamicDraw);
*/
          /*
          mVbo->bind();
          mVbo->allocate(NULL, Chunk::totalSize*sizeof(cSimpleVertex)*6*4);
          mVData = (cSimpleVertex*)mVbo->map(QOpenGLBuffer::WriteOnly);
          
          mIbo->bind();
          mIbo->allocate(NULL, Chunk::totalSize*sizeof(unsigned int)*6*4);
          mIData = (unsigned int*)mIbo->map(QOpenGLBuffer::WriteOnly);
          mIbo->release();
          */
          
          
	  //}
  
      mVao = new QOpenGLVertexArrayObject();
      if(!mVao->create())
	{
	  LOGE("VAO create failed in Mesh!!");
	  return false;
	}
      mVao->bind();
      mVbo->bind();
      mIbo->bind();
      if(mVertices.size() > 0 && mNeedUpdate & UPDATE_VERTICES)
        {
          mVbo->allocate(mVertices.data(), mVertices.size()*sizeof(cTexVertex));
          mNeedUpdate &= ~UPDATE_VERTICES;
        }
      if(mIndices.size() > 0 && mNeedUpdate & UPDATE_INDICES)
        {
          mIbo->allocate(mIndices.data(), mIndices.size()*sizeof(unsigned int));
          mNeedUpdate &= ~UPDATE_INDICES;
        }

      // configure attributes
      shader->setAttrBuffer(0, GL_FLOAT, 0, 3, sizeof(cTexVertex) );
      shader->setAttrBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, sizeof(cTexVertex) );
      shader->setAttrBuffer(2, GL_FLOAT, 6 * sizeof(float), 2, sizeof(cTexVertex) );
  
      mVao->release();
      //mVbo->release();
      //mIbo->release();
      
      //LOGD("done");
      mLoaded = true;
      return true;
    }
  else
    { return false; }
}

void Mesh::detachData()
{
  //GLuint vboId = mVbo->bufferId();
  //GLuint iboId = mIbo->bufferId();
  mVbo->bind();
  glBufferData(GL_VERTEX_ARRAY, 0, NULL, GL_DYNAMIC_DRAW);
  mIbo->bind();
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);

  //mVbo->release();
  //mIbo->release();
}

/*
void Mesh::setUpdatedData()
{
  
}
*/
bool Mesh::initialized() const
{ return mLoaded; }

void Mesh::render(Shader *shader)
{
  //std::lock_guard<std::mutex> lock(*mRenderLock);
  if(mVertices.size() > 0 && mIndices.size() > 0)
    {
      mVao->bind();
      if(mNeedUpdate & UPDATE_VERTICES)
        {
          mVbo->bind();
          mVbo->allocate(mVertices.data(), mVertices.size()*sizeof(cTexVertex));
          mNeedUpdate &= ~UPDATE_VERTICES;
        }
      if(mNeedUpdate & UPDATE_INDICES)
        {
          mIbo->bind();
          mIbo->allocate(mIndices.data(), mIndices.size()*sizeof(unsigned int));
          mNeedUpdate &= ~UPDATE_INDICES;
        }
      //LOGD("RENDERING MESH: '%s'", mName.c_str());
      glDrawElements(mMode, mIndices.size(), GL_UNSIGNED_INT, 0);
      //mIbo->release();
      //mVbo->release();
      mVao->release();
  }
}
