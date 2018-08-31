#ifndef MESH_HPP
#define MESH_HPP

#include "vertex.hpp"
#include "shader.hpp"
#include "objLoader.hpp"
#include <vector>
#include <string>
#include <mutex>

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_3_Core>

enum update_t
  {
   UPDATE_NONE = 0x00,
   UPDATE_VERTICES = 0x01,
   UPDATE_INDICES = 0x02,
  };

inline update_t& operator|=(update_t &u1, const update_t &u2) { u1 = (update_t)((int)u1 | (int)u2); return u1;}
inline update_t operator|(update_t u1, update_t u2) { return (update_t)((int)u1 | (int)u2); }
inline update_t& operator&=(update_t &u1, const update_t &u2) { u1 = (update_t)((int)u1 & (int)u2); return u1; }
inline update_t operator&(update_t u1, update_t u2) { return (update_t)((int)u1 & (int)u2); }
inline update_t operator~(update_t u1) { return (update_t)~((int)u1); }

class cMesh : protected QOpenGLFunctions_4_3_Core
{
public:
  cMesh();//bool doubleBuffered = true);
  cMesh(const objl::Mesh &mesh);//, bool doubleBuffered = true);
  ~cMesh();

  void setMode(GLenum mode);

  bool setMesh(const std::vector<objl::Vertex> &vertices,
	       const std::vector<unsigned int> &indices );
  
  bool setMesh(const std::vector<cSimpleVertex> &vertices,
	       const std::vector<unsigned int> &indices );

  bool setVertices(const std::vector<cSimpleVertex> &vertices);
  bool setIndices(const std::vector<unsigned int> &indices);

  void setUpdate()
  {
    //std::lock_guard<std::mutex> lock(*mRenderLock);
    //mCurrVbo = mDB ? (mCurrVbo+1)%2 : mCurrVbo;
    mNeedUpdate |= UPDATE_VERTICES | UPDATE_INDICES;
  }

  //void mmapWrite(
  
  const std::vector<cSimpleVertex>& getVertices() const
  { return mVertices; }
  //{ return mVertices[mDB ? (mCurrVbo+1)%2 : mCurrVbo]; }
  const std::vector<unsigned int>& getIndices() const
  { return mIndices; }
  //{ return mIndices[mDB ? (mCurrVbo+1)%2 : mCurrVbo]; }
  std::vector<cSimpleVertex>& getVertices()
  { return mVertices; }
  //{ return mVertices[mDB ? (mCurrVbo+1)%2 : mCurrVbo]; }
  std::vector<unsigned int>& getIndices()
  { return mIndices; }
  //{ return mIndices[mDB ? (mCurrVbo+1)%2 : mCurrVbo]; }

  void detachData();

  bool initialized() const;
  
  bool initGL(Shader *shader);
  void cleanupGL();
  void render(Shader *shader);
  
private:
  std::string mName;
  objl::Material mMaterial;
  std::vector<cSimpleVertex> mVertices;//[2];
  std::vector<unsigned int> mIndices;//[2];

  cSimpleVertex *mVData = nullptr;
  unsigned int *mIData = nullptr;

  bool mLoaded = false;
  //std::mutex *mRenderLock;
  
  QOpenGLBuffer *mVbo;//[2];
  QOpenGLBuffer *mIbo;//[2];
  QOpenGLBuffer *mVbo2;//[2];
  QOpenGLBuffer *mIbo2;//[2];
  //int mCurrVbo = 0;
  //bool mDB;
  
  QOpenGLVertexArrayObject *mVao;
  update_t mNeedUpdate = UPDATE_NONE;
  GLenum mMode = GL_TRIANGLES;
};


#endif // MESH_HPP
