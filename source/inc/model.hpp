#ifndef MODEL_HPP
#define MODEL_HPP

#include "vertex.hpp"
#include "mesh.hpp"
#include "objLoader.hpp"
#include "shader.hpp"
#include "geometry.hpp"
#include <vector>
#include <string>

class cModelObj
{
public:
  cModelObj(const std::string &filePath = "");
  cModelObj(const cModelObj &other) = default;
  bool initGL(cShader *shader);
  void cleanupGL();
  void render(cShader *shader, const Matrix4 &pvm);

  void setMode(GLenum mode);
  
  const std::vector<cSimpleVertex>* getVertices() const
  { return (mMeshes.size() > 0 ? &mMeshes[0].getVertices() : nullptr); }
  const std::vector<unsigned int>* getIndices() const
  { return (mMeshes.size() > 0 ? &mMeshes[0].getIndices() : nullptr); }
private:
  std::vector<cMesh> mMeshes;
  
  bool loadModel(const std::string &path);
};

#endif // MODEL_HPP
