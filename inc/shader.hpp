#ifndef SHADER_HPP
#define SHADER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <QOpenGLWidget>
#include <QObject>

#include "geometry.hpp"

class QWidget;
class QOpenGLShaderProgram;

class cShader : public QObject
{
  Q_OBJECT
public:
  cShader(QObject *parent);
  virtual ~cShader() { };
  bool loadProgram(const std::string &vshFile,
		   const std::string &fshFile,
		   const std::vector<std::string> &attributes,
		   const std::vector<std::string> &uniforms );
  void bind();
  void release();
  void setAttrBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0);
  void setUniform(const std::string &name, const Matrix4 &mat);
  void setUniform(const std::string &name, const QMatrix4x4 &mat);
  void setUniform(const std::string &name, int val);
  
private:
  bool mLoaded = false;
  QOpenGLShaderProgram *mProgram = nullptr;

  std::vector<std::string> mAttributes;
  std::unordered_map<std::string, int> mUniforms;
};

#endif // SHADER_HPP
