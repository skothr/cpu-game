#ifndef SHADER_HPP
#define SHADER_HPP

//#include <GL/glew.h>
//#include <GL/gl.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <QOpenGLFunctions_4_3_Core>
#include <QObject>

#include "geometry.hpp"

//class QOpenGLShaderProgram;

class cShader : public QObject, protected QOpenGLFunctions_4_3_Core
{
  Q_OBJECT
public:
  cShader();
  cShader(QObject *parent);
  virtual ~cShader() { };
  bool loadProgram(const std::string &vshFile,
		   const std::string &fshFile,
		   const std::vector<std::string> &attributes,
		   const std::vector<std::string> &uniforms );
  void bind();
  void release();
  void setAttrBuffer(int location, GLenum type, int offset, int tupleSize, int stride);
  void setAttrBufferInt(int location, GLenum type, int offset, int tupleSize, int stride);
  void setUniform(const std::string &name, const Matrix4 &mat);
  void setUniform(const std::string &name, const QMatrix4x4 &mat);
  void setUniform(const std::string &name, int val);
  void setUniform(const std::string &name, float val);
  void setUniform(const std::string &name, const Point3f &v);

  bool initialized() const
  { return mLoaded; }
  
private:
  bool mLoaded = false;
  //QOpenGLShaderProgram *mProgram = nullptr;

  GLuint mProgramId = 0;
  GLuint mVertShaderId = 0;
  GLuint mFragShaderId = 0;
  
  std::vector<std::string> mAttributes;
  std::unordered_map<std::string, int> mUniforms;

  bool compileShader(GLenum type, const std::string &filePath, GLuint &shaderIdOut);
  bool linkProgram();
};

#endif // SHADER_HPP
