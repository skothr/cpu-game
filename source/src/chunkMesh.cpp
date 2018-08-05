// #include "chunkMesh.hpp"


// cChunkMesh::cChunkMesh()//bool doubleBuffered)
//   : mName("")//, mRenderLock(new std::mutex()), mDB(doubleBuffered)
// {

// }

// /*
// cChunkMesh::cChunkMesh(const objl::Mesh &mesh)//, bool doubleBuffered)
//   : mName(mesh.MeshName)//, mMaterial(mesh.MeshMaterial)
//     //mRenderLock(new std::mutex()), mDB(doubleBuffered)
// {
//   //setMesh(mesh.Vertices,
//   //	  std::vector<unsigned int>(mesh.Indices.begin(), mesh.Indices.end() ));
// }
// */
// cChunkMesh::~cChunkMesh()
// {
//   //delete mRenderLock;
// }

// void cChunkMesh::setMode(GLenum mode)
// {
//   mMode = mode;
// }
// /*
// bool cChunkMesh::setMesh(const std::vector<objl::Vertex> &vertices,
// 		    const std::vector<unsigned int> &indices )
// {
//   mVertices.clear();
//   mVertices.reserve(vertices.size());
//   for(auto &v : vertices)
//     {
//       mVertices.push_back({{v.Position.X, v.Position.Y, v.Position.Z},
// 				     {v.Normal.X, v.Normal.Y, v.Normal.Z},
// 				     {v.TextureCoordinate.X, v.TextureCoordinate.Y }});
//     }

//   mIndices = indices;
//   //std::lock_guard<std::mutex> lock(*mRenderLock);
//   mNeedUpdate |= UPDATE_VERTICES | UPDATE_INDICES;
//   //mCurrVbo = (mCurrVbo + 1) % 2;
//   return true;
// }
// bool cChunkMesh::setMesh(const std::vector<cSimpleVertex> &vertices,
// 		    const std::vector<unsigned int> &indices )
// {
//   mVertices = vertices;
//   mIndices = indices;
//   //std::lock_guard<std::mutex> lock(*mRenderLock);
//   mNeedUpdate |= UPDATE_VERTICES | UPDATE_INDICES;
//   //mCurrVbo = (mCurrVbo + 1) % 2;
//   return true;
// }
// bool cChunkMesh::setVertices(const std::vector<cSimpleVertex> &vertices)
// {
//   mVertices = vertices;
//   //std::lock_guard<std::mutex> lock(*mRenderLock);
//   mNeedUpdate |= UPDATE_VERTICES;
//   // mCurrVbo = (mCurrVbo + 1) % 2;
//   return true;
// }
// bool cChunkMesh::setIndices(const std::vector<unsigned int> &indices)
// {
//   mIndices = indices;
//   //std::lock_guard<std::mutex> lock(*mRenderLock);
//   mNeedUpdate |= UPDATE_INDICES;
//   //mCurrVbo = (mCurrVbo + 1) % 2;
//   return true;
// }
// */
// void cChunkMesh::cleanupGL()
// {
//   mVao->destroy();
//   delete mVao;
//   /*
//   for(int i = 0; i < 2; i++)
//     {
//       mVbo[i]->destroy();
//       delete mVbo[i];
//       mIbo[i]->destroy();
//       delete mIbo[i];
//     }
//   */
//   mVbo->destroy();
//   delete mVbo;
//   mIbo->destroy();
//   delete mIbo;
// }

// // make sure to call this from the OpenGL thread!
// bool cChunkMesh::initGL(cShader *shader)
// {
//   if(!mLoaded)
//     {
//       //for(int i = 0; i < 2; i++)
//       //{
// 	  mVbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
// 	  if(!mVbo->create())
// 	    {
// 	      LOGE("VBO create failed in cChunkMesh!!");
// 	      return false;
// 	    }
// 	  mVbo->bind();
// 	  mVbo->setUsagePattern(QOpenGLBuffer::DynamicDraw);

// 	  mIbo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
// 	  if(!mIbo->create())
// 	    {
// 	      LOGE("IBO create failed in cChunkMesh!!");
// 	      return false;
// 	    }
// 	  mIbo->bind();
// 	  mIbo->setUsagePattern(QOpenGLBuffer::DynamicDraw);

//           /*
// 	  if(mVertices.size() > 0 && mNeedUpdate & UPDATE_VERTICES)
// 	    {
// 	      mVbo->allocate(mVertices.data(), mVertices.size()*sizeof(cSimpleVertex));
// 	      mNeedUpdate &= ~UPDATE_VERTICES;
// 	    }
// 	  if(mIndices.size() > 0 && mNeedUpdate & UPDATE_INDICES)
// 	    {
// 	      mIbo->allocate(mIndices.data(), mIndices.size()*sizeof(unsigned int));
// 	      mNeedUpdate &= ~UPDATE_INDICES;
// 	    }
//           */
// 	  //}
  
//       mVao = new QOpenGLVertexArrayObject();
//       if(!mVao->create())
// 	{
// 	  LOGE("VAO create failed in cChunkMesh!!");
// 	  return false;
// 	}
//       mVao->bind();

//       // configure attributes
//       shader->setAttrBuffer(0, GL_FLOAT, 0, 3, sizeof(cSimpleVertex) );
//       shader->setAttrBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, sizeof(cSimpleVertex) );
//       shader->setAttrBuffer(2, GL_FLOAT, 6 * sizeof(float), 3, sizeof(cSimpleVertex) );
  
//       mVao->release();
//       mVbo->release();
//       mIbo->release();
      
//       //LOGD("done");
//       mLoaded = true;
//       return true;
//     }
//   else
//     { return false; }
// }

// void detachData()
// {
//   //GLuint vboId = mVbo->bufferId();
//   //GLuint iboId = mIbo->bufferId();
//   mVbo->bind();
//   glBufferData(GL_VERTEX_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
//   mIbo->bind();
//   glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);

//   mVbo->release();
//   mIbo->release();
// }

// void setData(const std::vector<cSimpleVertex> *newVertices, const std::vector<unsigned int> *newIndices)
// {
//   mVbo->bind();
//   glBufferData(GL_VERTEX_BUFFER, newVertices->size()*sizeof(cSimpleVertex), newVertices->data(), GL_DYNAMIC_DRAW);
//   mIbo->bind();
//   glBufferData(GL_ELEMENT_ARRAY_BUFFER, newIndices->size()*sizeof(unsigned int), newIndices->data(), GL_DYNAMIC_DRAW);
  
//   mVbo->release();
//   mIbo->release();
// }


// bool cChunkMesh::initialized() const
// { return mLoaded; }

// void cChunkMesh::render(cShader *shader)
// {
//   //std::lock_guard<std::mutex> lock(*mRenderLock);
//   if(mVertices.size() > 0 && mIndices.size() > 0)
//   {
//     mVao->bind();
//     mVbo->bind();
//     mIbo->bind();
//     /*
//     if(mNeedUpdate & UPDATE_VERTICES)
//       {
// 	//LOGD("Updating vertices...");
// 	mVbo->allocate(mVertices.data(), mVertices.size()*sizeof(cSimpleVertex));
// 	mNeedUpdate &= ~UPDATE_VERTICES;
// 	//LOGD("NEED UPDATE: 0x%02X", mNeedUpdate);
//       }
//     if(mNeedUpdate & UPDATE_INDICES)
//       {
// 	//LOGD("Updating indices...");
// 	mIbo->allocate(mIndices.data(), mIndices.size()*sizeof(unsigned int));
// 	mNeedUpdate &= ~UPDATE_INDICES;
// 	//LOGD("NEED UPDATE: 0x%02X", mNeedUpdate);
//       }
//     */
//     glDrawElements(mMode, mIndices.size(), GL_UNSIGNED_INT, 0);
//     mIbo->release();
//     mVbo->release();
//     mVao->release();
//   }
// }
