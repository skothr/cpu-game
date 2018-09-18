#include "computeShader.hpp"
#include <qopengl.h>

#include "logging.hpp"
#include <sstream>
#include <fstream>

ComputeShader::ComputeShader()
{ }
ComputeShader::~ComputeShader()
{ }

void ComputeShader::bind()
{
  glUseProgram(mProgramId);
}
void ComputeShader::release()
{
  glUseProgram(0);
}


bool ComputeShader::compileShader(const std::string &filePath, GLuint &shaderIdOut)
{
  // vertex shader
  LOGI(" Compiling compute shader ('%s')...", filePath.c_str() );
  shaderIdOut = glCreateShader(GL_COMPUTE_SHADER);
  
  std::ifstream file(filePath, std::ios::in);
  std::ostringstream src;
  src << file.rdbuf();
  std::string srcStr = src.str();
  const char *fileData = srcStr.c_str();
  glShaderSource(shaderIdOut, 1, &fileData, NULL);
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

bool ComputeShader::linkProgram()
{
  LOGD("Linking compute shader program...");
  glAttachShader(mProgramId, mShaderId);
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
      glDeleteShader(mShaderId);
      return false;
    }

  // clean up
  glDetachShader(mProgramId, mShaderId);
  return true;
}

bool ComputeShader::loadProgram(const std::string &cshFile,
                                //const std::vector<std::string> &attributes,
                                const std::vector<std::string> &uniforms )
{
  if(!mInitialized)
    {
      initializeOpenGLFunctions();
      // get work groups
      glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &mNumWorkGroups[0]);
      glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &mNumWorkGroups[1]);
      glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &mNumWorkGroups[2]);
      LOGD("Compute work groups count: (%d, %d, %d)",
           mNumWorkGroups[0], mNumWorkGroups[1], mNumWorkGroups[2] );
      glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &mWorkGroupSize[0]);
      glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &mWorkGroupSize[1]);
      glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &mWorkGroupSize[2]);
      LOGD("Compute work group size:   (%d, %d, %d)",
           mWorkGroupSize[0], mWorkGroupSize[1], mWorkGroupSize[2] );

      if(!compileShader(cshFile, mShaderId))
        { return false; }
  
      mProgramId = glCreateProgram();
      bind();
  
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

      LOGI("Compute shader program compiled and linked.");
      mInitialized = true;
    }
  return true;
}


void ComputeShader::dispatch(const Point3i &blocks)
{
  bind();
  glDispatchCompute((GLuint)blocks[0], (GLuint)blocks[1], (GLuint)blocks[2]);
  release();
}

void ComputeShader::waitForFinish()
{
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

/*
void ComputeShader::setAttrBuffer(int location, GLenum type, int offset,
			    int tupleSize, int stride )
{
  glEnableVertexAttribArray(location);
  glVertexAttribPointer(location, tupleSize, type, GL_FALSE, stride, (GLvoid*)offset);
}
void ComputeShader::setAttrBufferInt(int location, GLenum type, int offset,
			       int tupleSize, int stride )
{
  glEnableVertexAttribArray(location);
  glVertexAttribIPointer(location, tupleSize, type, stride, (GLvoid*)offset);
}
*/
void ComputeShader::setUniform(const std::string &name, const Matrix4 &mat)
{
  glUniformMatrix4fv(mUniforms[name], 1, GL_TRUE, (GLfloat*)mat.data());
}

void ComputeShader::setUniform(const std::string &name, const QMatrix4x4 &qm)
{
  glUniformMatrix4fv(mUniforms[name], 1, GL_TRUE, (GLfloat*)qm.data());
}

void ComputeShader::setUniform(const std::string &name, int val)
{
  glUniform1i(mUniforms[name], val);
}
void ComputeShader::setUniform(const std::string &name, float val)
{
  glUniform1f(mUniforms[name], val);
}

void ComputeShader::setUniform(const std::string &name, const Point3f &v)
{
  glUniform3f(mUniforms[name], v[0], v[1], v[2]);
}

void ComputeShader::setUniform(const std::string &name, const Point2f &v)
{
  glUniform2f(mUniforms[name], v[0], v[1]);
}
