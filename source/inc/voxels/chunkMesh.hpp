#ifndef CHUNK_MESH_HPP
#define CHUNK_MESH_HPP

#include "vertex.hpp"
#include "meshData.hpp"
#include "logging.hpp"
#include <vector>
#include <mutex>

#include <QOpenGLFunctions_4_3_Core>

class QOpenGLBuffer;
class QOpenGLVertexArrayObject;
class cShader;
class cMeshBuffer;

class cChunkMesh : protected QOpenGLFunctions_4_3_Core
{
public:
  cChunkMesh(bool doubleBuffered = true);
  ~cChunkMesh();

  bool initialized() const;
  bool initGL(cShader *shader);
  void cleanupGL();
  void render();
  
  void uploadData(const MeshData &data);
  void detachData();

  //void startUpdating(cSimpleVertex* &verticesOut, unsigned int* &indicesOut, int maxSize);
  //void finishUpdating(int numIndices, std::vector<cSimpleVertex> &vData, std::vector<unsigned int> &iData);

private:
  std::mutex mBufferLock;
  bool mLoaded = false;
  bool mDB;

  int mActiveBuffer = 0;
  cMeshBuffer *mBuffers[2] = {nullptr, nullptr};

  cMeshBuffer* activeBuffer() { return mBuffers[mActiveBuffer]; }
  cMeshBuffer* inactiveBuffer() { return mBuffers[(mActiveBuffer+1)%2]; }
  void swapBuffers() { mActiveBuffer = (mActiveBuffer + 1) % 2; }
  
};

#endif // CHUNK_MESH_HPP
