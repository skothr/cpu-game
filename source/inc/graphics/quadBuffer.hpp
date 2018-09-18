#ifndef QUAD_BUFFER_HPP
#define QUAD_BUFFER_HPP

#include "vertex.hpp"
#include <array>
#include <QOpenGLFunctions_4_3_Core>

class QOpenGLBuffer;
class QOpenGLVertexArrayObject;
class Shader;

class QuadBuffer : protected QOpenGLFunctions_4_3_Core
{
public:
  QuadBuffer();
  ~QuadBuffer();

  bool initialized() const;
  bool initGL(Shader *shader);
  void cleanupGL();
  void render();

private:
  static const std::array<QuadVertex, 6> quadVertices;
  
  bool mInitialized = false;
  QOpenGLBuffer *mVbo = nullptr;
  QOpenGLVertexArrayObject *mVao = nullptr;
};


#endif // QUAD_BUFFER_HPP
