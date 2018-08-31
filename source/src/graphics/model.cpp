#include "model.hpp"

cModelObj::cModelObj(const std::string &filePath)
{
  if(filePath != "")
    loadModel(filePath);
}

void cModelObj::setMode(GLenum mode)
{
  for(auto &m : mMeshes)
    {
      m.setMode(mode);
    }
}

bool cModelObj::initGL(Shader *shader)
{
  for(int i = 0; i < mMeshes.size(); i++)
    { mMeshes[i].initGL(shader); }
  return true;
}

void cModelObj::cleanupGL()
{
  for(int i = 0; i < mMeshes.size(); i++)
    { mMeshes[i].cleanupGL(); }
}


void cModelObj::render(Shader *shader, const Matrix4 &pvm)
{
  //shader->bind();
  shader->setUniform("pvm", pvm);
  for(auto &m : mMeshes)
    { m.render(shader); }
  //shader->release();
}


bool cModelObj::loadModel(const std::string &filePath)
{
  objl::Loader loader;

  if(loader.LoadFile(filePath))
    {
      //mMeshes.reserve(loader.LoadedMeshes.size());
      for(auto &m : loader.LoadedMeshes)
	{ mMeshes.emplace_back(m); }
      return true;
    }
  else
    {
      LOGE("Failed to load model file '%s'.\n", filePath.c_str());
      return false;
    }
}
