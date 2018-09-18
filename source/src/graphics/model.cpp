#include "model.hpp"

ModelObj::ModelObj(const std::string &filePath)
{
  if(filePath != "")
    { loadModel(filePath); }
}

void ModelObj::setMode(GLenum mode)
{
  for(auto &m : mMeshes)
    {
      m.setMode(mode);
    }
}

bool ModelObj::initGL(Shader *shader)
{
  for(int i = 0; i < mMeshes.size(); i++)
    { mMeshes[i].initGL(shader); }
  return true;
}

void ModelObj::cleanupGL()
{
  for(int i = 0; i < mMeshes.size(); i++)
    { mMeshes[i].cleanupGL(); }
}


void ModelObj::render(Shader *shader, const Matrix4 &pvm)
{
  shader->setUniform("pvm", pvm);
  for(auto &m : mMeshes)
    { m.render(shader); }
}

bool ModelObj::loadModel(const std::string &filePath)
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
