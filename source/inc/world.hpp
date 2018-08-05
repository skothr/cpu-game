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
  cWorld(const std::string &worldName, int numThreads,
	 const Point3i &center, const Vector3i &chunkRadius );
  ~cWorld();

  block_t get(const Point3i &p);
  bool set(const Point3i &p, block_t type);
  
  cChunk* getChunk(const Point3i &p);

  void setSeed(uint32_t seed);
  
  bool rayCast(const Point3f &p, const Vector3f &d, float radius,
	       block_t &blockOut, Point3i &posOut, Vector3i &faceOut );
  
  static void loadModels();
  static void modelInitGL(cShader *complexShader);
  static void modelCleanupGL();
  void initGL(cShader *simpleShader);//, cShader *complexShader);
  void cleanupGL();
  void renderSimple(cShader *shader, Matrix4 pvm);
  void renderComplex(cShader *shader, Matrix4 pvm);

  Point3i chunkPos(const Point3f &worldPos) const;
  void setCenter(const Point3i &chunkCenter);
  Point3i getCenter() const;
  
  void startLoading();
  void stopLoading();
  
  void update();

  bool readyForPlayer() const;
  Point3f getStartPos(const Point3i &pPos);
  
private:
  // 2D -- chunks only spawn horizontally (future idea: since it's space, add vertical generation)
  // mData[Y][X]
  cChunkManager mChunks;
  bool mInitialized = false;
  //int mNumThreads;
  //bool mThreadsRunning = true;
  //std::vector<std::thread> mThreads;
  //std::vector<std::vector<Point3i>> mThreadChunks;
  //std::vector<bool> mThreadsDone;
  //int mThreadsWorking = 0;

  // std::mutex mThreadWorkLock;
  // std::mutex mThreadWaitLock;
  // std::condition_variable mThreadWorkCv;
  // std::condition_variable mThreadWaitCv;

  // std::vector<std::array<std::vector<SimpleFace>, BLOCK_SIMPLE_COUNT>> mThreadFaces;
  // std::vector<std::array<std::vector<Point3i>, BLOCK_COMPLEX_COUNT>> mThreadComplex;
  
  static std::array<cModelObj, BLOCK_COMPLEX_COUNT> mModels;
  static std::array<cSimpleVertex, FACE_VERTICES> mFaceVertices[(int)normal_t::COUNT];
  static std::array<unsigned int, FACE_INDICES> mFaceIndices;
  
  // std::array<cMesh, BLOCK_SIMPLE_COUNT> mSimpleMeshes;
  // std::array<cMesh, BLOCK_COMPLEX_COUNT> mComplexMeshes;
  // bool mNeedUpdate = true;
  
  void updateMesh();
  void chunkWorker(int id);
};

#endif // WORLD_HPP
