#include "shader.hpp"
#include <QWidget>
#include <QOpenGLShaderProgram>

#include "logging.hpp"
#include <sstream>
#include <fstream>

cShader::cShader(QObject *parent)
  : QObject(parent)
{

}

void cShader::bind()
{
  mProgram->bind();
}
void cShader::release()
{
  mProgram->release();
}


bool cShader::loadProgram(const std::string &vshFile, const std::string &fshFile,
			  const std::vector<std::string> &attributes,
			  const std::vector<std::string> &uniforms )
{
  mLoaded = false;
  
  LOGD("Loading shader program...\n");
  mProgram = new QOpenGLShaderProgram(this);

  // vertex shader
  LOGI(" Compiling vertex shader ('%s')...", vshFile.c_str());
  {
    std::ifstream file(vshFile, std::ios::in);
    std::ostringstream src;
    src << file.rdbuf();
    if(!mProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, QString::fromStdString(src.str())))
      {
	qDebug() << mProgram->log();
	return false;
      }
  }
  // fragment shader
  LOGI(" Compiling fragment shader('%s')...", fshFile.c_str());
  {
    std::ifstream file(fshFile, std::ios::in);
    std::ostringstream src;
    src << file.rdbuf();
    if(!mProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, QString::fromStdString(src.str())))
      {
	qDebug() << mProgram->log();
	return false;
      }
  }

  // shader attribues
  mAttributes = attributes;
  LOGI(" Binding attribute locations...\n");
  for(int i = 0; i < mAttributes.size(); i++)
    {
      mProgram->bindAttributeLocation(mAttributes[i].c_str(), i);
      mProgram->enableAttributeArray(i);
      LOGD(" Attribute %d --> '%s'\n", i, mAttributes[i].c_str());
    }

  // link shader program
  LOGD(" Linking shader program...");
  if(!mProgram->link())
    {
      qDebug() << mProgram->log();
      return false;
    }
  
  
  // uniforms
  LOGI(" Getting uniform locations...\n");
  for(int i = 0; i < uniforms.size(); i++)
    {
      int loc = mProgram->uniformLocation(uniforms[i].c_str());
      mUniforms.emplace(uniforms[i], loc);
      LOGD(" Uniform '%s' at %d\n", uniforms[i].c_str(), loc);
    }
  
  LOGI("Shader program compiled and linked.\n");
  mLoaded = true;
  return true;
}

void cShader::setAttrBuffer(int location, GLenum type, int offset,
			    int tupleSize, int stride )
{
  //LOGD("Setting attr buffer...");
  mProgram->enableAttributeArray(location);
  mProgram->setAttributeBuffer(location, type, offset, tupleSize, stride);
}

void cShader::setUniform(const std::string &name, const Matrix4 &mat)
{
  QMatrix4x4(mat.data());
  mProgram->setUniformValue(mUniforms[name], QMatrix4x4(mat.data()));
}

void cShader::setUniform(const std::string &name, const QMatrix4x4 &qm)
{
  mProgram->setUniformValue(mUniforms[name], qm);
}

void cShader::setUniform(const std::string &name, int val)
{
  mProgram->setUniformValue(mUniforms[name], val);
}
