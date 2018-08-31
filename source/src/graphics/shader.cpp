#include "shader.hpp"
#include <QWidget>
//#include <QOpenGLShaderProgram>
//#include <qopengl.h>

#include "logging.hpp"
#include <sstream>
#include <fstream>

Shader::Shader()
{

}

Shader::Shader(QObject *parent)
  : QObject(parent)
{

}

void Shader::bind()
{
  //mProgram->bind();
  glUseProgram(mProgramId);
}
void Shader::release()
{
  //mProgram->release();
  glUseProgram(0);
}


bool Shader::compileShader(GLenum type, const std::string &filePath, GLuint &shaderIdOut)
{
  // vertex shader
  LOGI(" Compiling %s shader ('%s')...",
       (type == GL_VERTEX_SHADER ? "vertex" :
	(type == GL_FRAGMENT_SHADER ? "fragment" :
	 "unsupported" )),
       filePath.c_str() );
  
  shaderIdOut = glCreateShader(type);
  
  std::ifstream file(filePath, std::ios::in);
  std::ostringstream src;
  src << file.rdbuf();
  std::string srcStr = src.str();
  const char *fileData = srcStr.c_str();
  glShaderSource(shaderIdOut, 1, &fileData, 0);
  glCompileShader(shaderIdOut);

  GLint compiled = 0;
  glGetShaderiv(shaderIdOut, GL_COMPILE_STATUS, &compiled);
  if(compiled == GL_FALSE)
    {
      GLint maxLength = 0;
      glGetShaderiv(shaderIdOut, GL_INFO_LOG_LENGTH, &maxLength);

      std::vector<GLchar> infoLog(maxLength);
      glGetShaderInfoLog(shaderIdOut, maxLength, &maxLength, &infoLog[0]);
      LOGE("Compile failed. Log:");
      LOGE("%s", infoLog.data());
	
      // clean up
      glDeleteShader(shaderIdOut);
      return false;
    }

  return true; 
}

bool Shader::linkProgram()
{
  LOGD("Linking shader program...");
  glAttachShader(mProgramId, mVertShaderId);
  glAttachShader(mProgramId, mFragShaderId);
  glLinkProgram(mProgramId);

  GLint linked = 0;
  glGetProgramiv(mProgramId, GL_LINK_STATUS, &linked);
  if (linked == GL_FALSE)
    {
      GLint maxLength = 0;
      glGetProgramiv(mProgramId, GL_INFO_LOG_LENGTH, &maxLength);

      std::vector<GLchar> infoLog(maxLength);
      glGetProgramInfoLog(mProgramId, maxLength, &maxLength, &infoLog[0]);
      LOGE("Linking failed. Log:");
      LOGE("%s", infoLog.data());

      // clean up
      glDeleteProgram(mProgramId);
      glDeleteShader(mVertShaderId);
      glDeleteShader(mFragShaderId);
      return false;
    }

  // clean up
  glDetachShader(mProgramId, mVertShaderId);
  glDetachShader(mProgramId, mFragShaderId);
  return true;
}

bool Shader::loadProgram(const std::string &vshFile, const std::string &fshFile,
			  const std::vector<std::string> &attributes,
			  const std::vector<std::string> &uniforms )
{
  if(!mLoaded)
    { initializeOpenGLFunctions(); }
  mLoaded = false;

  GLuint vShader = 0;
  if(!compileShader(GL_VERTEX_SHADER, vshFile, vShader))
    { return false; }

  GLuint fShader = 0;
  if(!compileShader(GL_FRAGMENT_SHADER, fshFile, fShader))
    {
      glDeleteShader(vShader);
      return false;
    }
  
  mVertShaderId = vShader;
  mFragShaderId = fShader;
  mProgramId = glCreateProgram();
  bind();
  
  // shader attribues
  mAttributes = attributes;
  LOGI(" Binding attribute locations...");
  for(int i = 0; i < mAttributes.size(); i++)
    {
      glBindAttribLocation(mProgramId, i, mAttributes[i].c_str());
      glEnableVertexAttribArray(i);
      LOGD(" Attribute %d --> '%s'\n", i, mAttributes[i].c_str());
    }
  
  // link shader program
  if(!linkProgram())
    { return false; }
  
  // uniforms
  LOGI(" Getting uniform locations...");
  for(int i = 0; i < uniforms.size(); i++)
    {
      int loc = glGetUniformLocation(mProgramId, uniforms[i].c_str());
      mUniforms.emplace(uniforms[i], loc);
      LOGD(" Uniform '%s' at %d\n", uniforms[i].c_str(), loc);
    }
  
  LOGI("Shader program compiled and linked.");
  mLoaded = true;
  return true;
}

void Shader::setAttrBuffer(int location, GLenum type, int offset,
			    int tupleSize, int stride )
{
  glEnableVertexAttribArray(location);
  glVertexAttribPointer(location, tupleSize, type, GL_FALSE, stride, (GLvoid*)offset);
}
void Shader::setAttrBufferInt(int location, GLenum type, int offset,
			       int tupleSize, int stride )
{
  glEnableVertexAttribArray(location);
  glVertexAttribIPointer(location, tupleSize, type, stride, (GLvoid*)offset);
}

void Shader::setUniform(const std::string &name, const Matrix4 &mat)
{
  glUniformMatrix4fv(mUniforms[name], 1, GL_TRUE, (GLfloat*)mat.data());
}

void Shader::setUniform(const std::string &name, const QMatrix4x4 &qm)
{
  glUniformMatrix4fv(mUniforms[name], 1, GL_TRUE, (GLfloat*)qm.data());
}

void Shader::setUniform(const std::string &name, int val)
{
  glUniform1i(mUniforms[name], val);
}
void Shader::setUniform(const std::string &name, float val)
{
  glUniform1f(mUniforms[name], val);
}

void Shader::setUniform(const std::string &name, const Point3f &v)
{
  glUniform3f(mUniforms[name], v[0], v[1], v[2]);
}
