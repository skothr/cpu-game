#ifndef BLOCK_MESH_HPP
#define BLOCK_MESH_HPP

#include "vertex.hpp"
#include "shader.hpp"
#include <vector>

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

class cBlockMesh
{
public:
  cBlockMesh();
  cBlockMesh(const std::vector<cSimpleVertex> &vertices);
  ~cBlockMesh();

  bool setMesh(const std::vector<cSimpleVertex> &vertices);
  
  const std::vector<cSimpleVertex>& getVertices() const { return mVertices; }
  std::vector<cSimpleVertex>& getVertices() { return mVertices; }
  
  bool initGL(cShader *shader);
  void cleanupGL();
  void render(cShader *shader);
  
  void setUpdate() { mNeedUpdate = true; }
  
  bool mUpdating = false; // true when being updated by the world.
  
private:
  std::vector<cSimpleVertex> mVertices;
  
  QOpenGLBuffer *mVbo;
  QOpenGLVertexArrayObject *mVao;
  bool mNeedUpdate = true;
};


#endif // BLOCK_MESH_HPP
