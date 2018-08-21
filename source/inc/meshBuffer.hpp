#ifndef MESH_BUFFER_HPP
#define MESH_BUFFER_HPP

#include "vertex.hpp"
#include <vector>
#include <mutex>

#include <QOpenGLFunctions_4_3_Core>

class QOpenGLBuffer;
class QOpenGLVertexArrayObject;
class cShader;

class cMeshBuffer : protected QOpenGLFunctions_4_3_Core
{
public:
  cMeshBuffer();
  ~cMeshBuffer();
  
  bool initGL(cShader *shader, const std::vector<cSimpleVertex> *vert = nullptr,
              const std::vector<unsigned int> *ind = nullptr );
  void cleanupGL();
  void render();

  void startUpdating(cSimpleVertex* &verticesOut, unsigned int* &indicesOut, int maxSize);
  void finishUpdating(int numIndices, std::vector<cSimpleVertex> &vData, std::vector<unsigned int> &iData);

  bool initialized() const;
  
  int numDraw() { return mNumDraw; }
  
private:
  bool mLoaded = false;
  int mMaxVertices = 0;
  int mNumDraw = 0;
  QOpenGLBuffer *mVbo = nullptr;
  QOpenGLBuffer *mIbo = nullptr;
  QOpenGLVertexArrayObject *mVao = nullptr;
};

#endif // MESH_BUFFER_HPP
