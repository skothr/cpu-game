// #ifndef CHUNK_MESH_HPP
// #define CHUNK_MESH_HPP


// class cChunkMesh
// {

//   cChunkMesh(bool doubleBuffered = true);
//   ~cChunkMesh();

//   /*
//   void setUpdate()
//   {
//     //std::lock_guard<std::mutex> lock(*mRenderLock);
//     //mCurrVbo = mDB ? (mCurrVbo+1)%2 : mCurrVbo;
//     mNeedUpdate |= UPDATE_VERTICES | UPDATE_INDICES;
//   }
  
//   const std::vector<cSimpleVertex>& getVertices() const
//   { return mVertices; }
//   //{ return mVertices[mDB ? (mCurrVbo+1)%2 : mCurrVbo]; }
//   const std::vector<unsigned int>& getIndices() const
//   { return mIndices; }
//   //{ return mIndices[mDB ? (mCurrVbo+1)%2 : mCurrVbo]; }
//   std::vector<cSimpleVertex>& getVertices()
//   { return mVertices; }
//   //{ return mVertices[mDB ? (mCurrVbo+1)%2 : mCurrVbo]; }
//   std::vector<unsigned int>& getIndices()
//   { return mIndices; }
//   //{ return mIndices[mDB ? (mCurrVbo+1)%2 : mCurrVbo]; }
//   */

//   void setData(const std::vector<cSimpleVertex> *newVertices, const std::vector<unsigned int> *newIndices);
  
//   bool initialized() const;
  
//   bool initGL(cShader *shader);
//   void cleanupGL();
//   void render(cShader *shader);
  
// private:
//   //std::string mName;
//   //objl::Material mMaterial;
//   std::vector<cSimpleVertex> mVertices;
//   std::vector<unsigned int> mIndices;

//   bool mLoaded = false;
//   //std::mutex *mRenderLock;
  
//   QOpenGLBuffer *mVbo;//[2];
//   QOpenGLBuffer *mIbo;//[2];
//   //int mCurrVbo = 0;
//   //bool mDB;
  
//   QOpenGLVertexArrayObject *mVao;
//   update_t mNeedUpdate = UPDATE_NONE;
//   GLenum mMode = GL_TRIANGLES;
// };

// #endif // CHUNK_MESH_HPP
