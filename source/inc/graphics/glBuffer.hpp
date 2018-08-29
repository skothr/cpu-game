#ifndef GL_BUFFER_HPP
#define GL_BUFFER_HPP

#include <QOpenGLFunctions_4_0_Core>

class cGlBuffer : protected QOpenGLFunctions_4_0_Core
{
public:
  cGlBuffer();
  ~cGlBuffer();

  bool create();
  void bind();
  void release();
  
private:
  bool mLoaded = false;
};


#endif // GL_BUFFER_HPP
