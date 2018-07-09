#ifndef CHUNKMAP_HPP
#define CHUNKMAP_HPP

#include "block.hpp"
#include "model.hpp"
#include "mesh.hpp"

#define SUBCHUNK_SIZE 8
#define CHUNK_SIZE 8
#define CHUNK_BLOCKS (SUBCHUNK_SIZE * CHUNK_SIZE)

class cSubChunk
{
public:
  static const int numElements = SUBCHUNK_SIZE * SUBCHUNK_SIZE * SUBCHUNK_SIZE;
  
  inline int getIndex(int x, int y, int z) const
  { return z*SUBCHUNK_SIZE*SUBCHUNK_SIZE + y*SUBCHUNK_SIZE + x; }
  
  cSubChunk();

  cBlock* operator()(int x, int y, int z);
  const cBlock* operator()(int x, int y, int z) const;
  cBlock* operator()(const Point3i &p);
  const cBlock* operator()(const Point3i &p) const;
  
  cBlock* data()
  { return mData; }
  const cBlock* data() const
  { return mData; }
  
  cBlock* get(const Point3i &p);
  const cBlock* get(const Point3i &p) const;
  
private:
  cBlock mData[numElements];
};


class cVirtualChunk
{
public:
  static const int numElements = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
  
  inline int getIndex(int x, int y, int z) const
  { return z*CHUNK_SIZE*CHUNK_SIZE + y*CHUNK_SIZE + x; }
  
  cVirtualChunk(const Point2i &offset);
  
  cSubChunk* operator()(int x, int y, int z);
  const cSubChunk* operator()(int x, int y, int z) const;
  cSubChunk* operator()(const Point3i &p);
  const cSubChunk* operator()(const Point3i &p) const;
  
  cSubChunk* data()
  { return mData; }
  const cSubChunk* data() const
  { return mData; }

  cBlock* get(const Point3i &p);
  const cBlock* get(const Point3i &p) const;
  
private:
  cSubChunk mData[numElements];
  Point2i mWorldOffset;

  Point3i subPoint(const Point3i &p) const;
};

class cShader;

class cChunkMap
{
public:
  cChunkMap(const Point2i &offset);
  ~cChunkMap();

  cBlock* get(const Point3i &p);
  const cBlock* get(const Point3i &p) const;

  bool set(const Point3i &p, block_t type);

  void loadChunk(const Point2i &chunkIndex);
  
  bool rayCast(const Point3f &p, const Vector3f &d, float radius,
	       cBlock *&blockOut, Point3i &posOut, Vector3i &faceOut );
  
  static void loadModels();
  static void modelInitGL(cShader *shader);
  static void modelCleanupGL();
  void initGL(cShader *shader);
  void cleanupGL();
  void render(cShader *shader, Matrix4 pvm);
  
private:
  // 2D -- chunks only spawn horizontally (future idea: since it's space, add vertical generation)
  std::vector<std::vector<cVirtualChunk*>> mData;
  Vector2i mShape;
  Point2i mWorldOffset;
  bool initialized = false;
  
  static std::vector<cModelObj> mModels;
  std::vector<cMesh> mMeshes;
  void updateMesh();
};

#endif // CHUNKMAP_HPP
