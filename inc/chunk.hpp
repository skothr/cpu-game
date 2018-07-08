#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <vector>
#include "geometry.hpp"
#include "block.hpp"
#include "vector.hpp"
#include "model.hpp"
#include "octree.hpp"
#include "mesh.hpp"
#include "collision.hpp"

class cShader;

class cChunk
{
public:
  cChunk(Vector3i size);
  ~cChunk();

  static void loadModels();
  static void modelInitGL(cShader *shader);
  static void modelCleanupGL();
  void initGL(cShader *shader);
  void cleanupGL();
  
  void render(cShader *shader, Matrix4 pvm);
  
  bool placeBlocks(const std::vector<Point3i> &pos, const std::vector<cBlock*> &blocks);
  bool placeBlocks(const std::vector<Point3i> &pos, const std::vector<block_t> &types);
  block_t pickBlock(const Point3i &pos);
  //bool closestIntersection(Point3f pos, Vector3f dir, Point3i &blockOut, Vector3i &faceOut)
  bool closestIntersection(const Point3f &pos, const Vector3f &dir, float radius, cBlock *&blockOut, Point3i &posOut, Vector3i &faceOut);

  std::vector<cBoundingBox> collides(const cBoundingBox &box, bool edge = false);
  std::vector<Vector3f> correction(const cBoundingBox &box, const std::vector<cBoundingBox> &block, const Vector3f &dPos, const Vector3i &onGround);
  
private:
  static std::vector<cModelObj> mModels;
  std::vector<cMesh> mMeshes;
  Vector3i mSize;
  //block_t *mBlocks;
  cOctree mBlocks;
  std::vector<cBoundingBox> mBlockBB;

  void updateMesh();
};


#endif // CHUNK_HPP
