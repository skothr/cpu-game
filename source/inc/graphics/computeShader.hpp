#ifndef COMPUTE_SHADER_HPP
#define COMPUTE_SHADER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <QOpenGLFunctions_4_3_Core>

#include "matrix.hpp"

class ComputeShader : protected QOpenGLFunctions_4_3_Core
{
public:
  ComputeShader();
  ~ComputeShader();
  bool loadProgram(const std::string &cshFile,
		   const std::vector<std::string> &uniforms );
  void bind();
  void release();
  void dispatch(const Point3i &blocks);
  void waitForFinish();
  
  void setUniform(const std::string &name, const Matrix4 &mat);
  void setUniform(const std::string &name, const QMatrix4x4 &mat);
  void setUniform(const std::string &name, int val);
  void setUniform(const std::string &name, float val);
  void setUniform(const std::string &name, const Point3f &v);
  void setUniform(const std::string &name, const Point2f &v);

  bool initialized() const
  { return mInitialized; }
  
private:
  bool mInitialized = false;
  
  GLuint mProgramId = 0;
  GLuint mShaderId = 0;
  
  Point3i mNumWorkGroups;
  Point3i mWorkGroupSize;
  
  std::unordered_map<std::string, int> mUniforms;

  bool compileShader(const std::string &filePath, GLuint &shaderIdOut);
  bool linkProgram();
};

#endif // COMPUTE_SHADER_HPP
