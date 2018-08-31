#ifndef MESH_BUFFER_HPP
#define MESH_BUFFER_HPP

#include "vertex.hpp"
#include "meshData.hpp"
#include <vector>
#include <mutex>

#include <QOpenGLFunctions_4_3_Core>

class QOpenGLBuffer;
class QOpenGLVertexArrayObject;
class Shader;

class cMeshBuffer : protected QOpenGLFunctions_4_3_Core
{
public:
  cMeshBuffer();
  ~cMeshBuffer();
  
  bool initialized() const;
  bool initGL(Shader *shader);
  void cleanupGL();
  void render();

  void setMode(GLenum mode) { mMode = mode; }
  
  void uploadData(const MeshData &data);
  int empty() { return mNumDraw == 0; }

  void detachData();
  
  //void startUpdating(cSimpleVertex* &verticesOut, unsigned int* &indicesOut, int maxSize);
  //void finishUpdating(int numIndices, std::vector<cSimpleVertex> &vData, std::vector<unsigned int> &iData);
private:
  bool mLoaded = false;
  int mMaxVertices = 0;
  int mNumDraw = 0;
  GLenum mMode = GL_TRIANGLES;
  QOpenGLBuffer *mVbo = nullptr;
  QOpenGLBuffer *mIbo = nullptr;
  QOpenGLVertexArrayObject *mVao = nullptr;
};

#endif // MESH_BUFFER_HPP
