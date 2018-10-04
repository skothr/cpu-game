#ifndef MODEL_HPP
#define MODEL_HPP

#include "vertex.hpp"
#include "mesh.hpp"
#include "objLoader.hpp"
#include "shader.hpp"
#include "matrix.hpp"
#include <vector>
#include <string>

class ModelObj
{
public:
  ModelObj(const std::string &filePath = "");
  ModelObj(const ModelObj &other) = default;
  bool initGL(Shader *shader);
  void cleanupGL();
  void render(Shader *shader, const Matrix4 &pvm);

  void setMode(GLenum mode);
  
  const std::vector<cTexVertex>* getVertices() const
  { return (mMeshes.size() > 0 ? &mMeshes[0].getVertices() : nullptr); }
  const std::vector<unsigned int>* getIndices() const
  { return (mMeshes.size() > 0 ? &mMeshes[0].getIndices() : nullptr); }
private:
  std::vector<Mesh> mMeshes;
  
  bool loadModel(const std::string &path);
};

#endif // MODEL_HPP
