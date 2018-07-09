#ifndef MESH_HPP
#define MESH_HPP

#include "vertex.hpp"
#include "shader.hpp"
#include "objLoader.hpp"
#include <vector>
#include <string>

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>


enum update_t
  {
   UPDATE_NONE = 0x00,
   UPDATE_VERTICES = 0x01,
   UPDATE_INDICES = 0x02,
  };

inline update_t& operator|=(update_t &u1, const update_t &u2) { u1 = (update_t)((int)u1 | (int)u2); return u1;}
inline update_t operator|(update_t u1, update_t u2) { return (update_t)((int)u1 | (int)u2); }
inline update_t& operator&=(update_t &u1, const update_t &u2) { u1 = (update_t)((int)u1 & (int)u2); return u1; }
inline update_t operator&(update_t u1, update_t u2) { return (update_t)((int)u1 & (int)u2); }
inline update_t operator~(update_t u1) { return (update_t)~((int)u1); }

class cMesh
{
public:
  cMesh();
  cMesh(const objl::Mesh &mesh);
  ~cMesh();

  void setMode(GLenum mode);

  bool setMesh(const std::vector<objl::Vertex> &vertices,
	       const std::vector<unsigned int> &indices );

  bool setVertices(const std::vector<objl::Vertex> &vertices);
  bool setIndices(const std::vector<unsigned int> &indices);

  void setUpdate() { mNeedUpdate |= UPDATE_VERTICES | UPDATE_INDICES; }
  
  const std::vector<objl::Vertex>& getVertices() const { return mVertices; }
  const std::vector<unsigned int>& getIndices() const { return mIndices; }
  std::vector<objl::Vertex>& getVertices() { return mVertices; }
  std::vector<unsigned int>& getIndices() { return mIndices; }
  
  bool initGL(cShader *shader);
  void cleanupGL();
  void render(cShader *shader);
  
private:
  std::string mName;
  objl::Material mMaterial;
  std::vector<objl::Vertex> mVertices;
  std::vector<unsigned int> mIndices;
  
  QOpenGLBuffer *mVbo;
  QOpenGLBuffer *mIbo;
  QOpenGLVertexArrayObject *mVao;
  update_t mNeedUpdate = UPDATE_NONE;
  GLenum mMode = GL_TRIANGLES;
};


#endif // MESH_HPP
