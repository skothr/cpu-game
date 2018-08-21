#ifndef WORLD_HPP
#define WORLD_HPP

#include "block.hpp"
#include "model.hpp"
#include "mesh.hpp"
//#include "hashChunk.hpp"
#include "chunkManager.hpp"

#include <array>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

struct SimpleFace
{
  Point3i pos;
  normal_t norm;
  block_t type;
};

#define FACE_VERTICES 4
#define FACE_INDICES 6

class cShader;

class cWorld
{
public:
  cWorld(const std::string &worldName, uint32_t seed, int numThreads,
	 const Point3i &center, const Vector3i &chunkRadius );
  ~cWorld();

  block_t get(const Point3i &p);
  bool set(const Point3i &p, block_t type);
  
  cChunk* getChunk(const Point3i &p);
  
  bool rayCast(const Point3f &p, const Vector3f &d, float radius,
	       block_t &blockOut, Point3i &posOut, Vector3i &faceOut );
  
  void initGL(QObject *qparent);
  void cleanupGL();
  void render(Matrix4 pvm);

  Point3i chunkPos(const Point3f &worldPos) const;
  void setCenter(const Point3i &chunkCenter);
  Point3i getCenter() const;
  
  void startLoading();
  void stopLoading();
  
  void update();

  bool readyForPlayer() const;
  Point3f getStartPos(const Point3i &pPos);
  
private:
  cChunkManager mChunks;
  bool mInitialized = false;
};

#endif // WORLD_HPP
