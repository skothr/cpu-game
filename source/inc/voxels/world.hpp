#ifndef WORLD_HPP
#define WORLD_HPP

#include "block.hpp"
#include "chunk.hpp"
#include "chunkMap.hpp"
#include "miscMath.hpp"
#include "meshing.hpp"
#include "terrain.hpp"
#include "fluidManager.hpp"
#include "hashing.hpp"

#include <mutex>
#include <vector>

class Shader;
class cTextureAtlas;
class FluidChunk;
class MeshRenderer;
class RayTracer;
class ModelObj;
class Camera;
class ChunkLoader;
class cMeshBuffer;
class ChunkVisualizer;

class World
{
public:
  World();
  ~World();
  
  struct Options
  {
    std::string name;
    terrain_t terrain;
    uint32_t seed;
    Point3i playerPos;
    Vector3i chunkRadius;
    int loadThreads;
    int meshThreads;
  };
  
  // loading
  std::vector<Options> getWorlds() const;
  bool worldExists(const std::string &name) const;
  bool loadWorld(Options &opt);
  bool createWorld(Options &opt, bool overwrite = false);
  bool deleteWorld(const std::string &worldName);
  void updateInfo(const Options &opt);
  void start();
  void stop();

  int totalChunks() const { return mChunkDim[0]*mChunkDim[1]*mChunkDim[2]; }
  int centerChunks() const { return (mChunkDim[0]-2)*(mChunkDim[1]-2)*(mChunkDim[2]-2); }
  int numLoaded() const;
  int numMeshed();

  bool chunkIsLoading(const Point3i &cp);
  bool chunkIsLoaded(const Point3i &cp);
  bool chunkIsMeshed(const Point3i &cp);
  bool chunkIsMeshing(const Point3i &cp);
  bool chunkIsEmpty(const Point3i &cp);
  bool chunkIsReady(const Point3i &cp);
  bool chunkIsVisible(const Point3i &cp);

  void rotateVisualizer(const Vector3f &angle);

  ChunkMap* getChunkMap() { return &mChunkMap; }

  void reset();

  // rendering
  bool initGL(QObject *qparent);
  void cleanupGL();
  void render(const Matrix4 &pvm);

  void setDebug(bool debug);
  void setFluidSim(bool on);
  void setFluidEvap(float rate);
  void setRaytracing(bool on);
  void clearFluids();

  void setCamera(Camera *camera);
  void setFrustumCulling(bool on);
  void pauseFrustumCulling();

  void setScreenSize(const Point2i &size);

  // updating
  void update();
  void step();
  bool readyForPlayer();
  Point3i getPlayerStartPos();
  Point3f getStartPos();
  void setPlayerPos(const Point3i &ppos);
  
  void setLightLevel(int level) { mLighting = level; }
  void setCenter(const Point3i &chunkCenter);
  void setRadius(const Vector3i &chunkRadius);
  Point3i getCenter() const;
  Vector3i getRadius() const { return mLoadRadius; }
  void setCamPos(const Point3f &pos) { mCamPos = pos; }

  block_t* at(const Point3i &wp);
  block_t getType(const Point3i &wp);
  ChunkPtr getChunk(const Point3i &wp);
  bool setBlock(const Point3i &p, block_t type, BlockData *data = nullptr);
  
  bool setRange(const Point3i &center, int rad, block_t type, BlockData *data=nullptr);
  bool setRange(const Point3i &p1, const Point3i &p2, block_t type, BlockData *data=nullptr);
  bool setSphere(const Point3i &center, int rad, block_t type, BlockData *data=nullptr);
  
  bool rayCast(const Point3f &p, const Vector3f &d, float radius,
	       CompleteBlock &blockOut, Point3i &posOut, Vector3i &faceOut );
  
  static int chunkX(int wx);
  static int chunkY(int wy);
  static int chunkZ(int wz);
  static Point3i chunkPos(const Point3i &wp);
  
private:
  // flags
  bool mInitialized = false;
  bool mRaytrace = false;
  bool mDebug = false;
  bool mSimFluids = true;
  float mEvapRate = 0.0;
  bool mResetGL = false;
  bool mFrustumPaused = false;

  // main objects
  ChunkMap mChunkMap;
  FluidManager mFluids;
  ChunkLoader *mLoader;
  MeshRenderer *mRenderer;
  RayTracer *mRayTracer;

  Camera *mCamera = nullptr;

  ChunkVisualizer *mVisualizer = nullptr;
  Vector3f mVisualizerRotate = Vector3f{0,0,0};

  // rendering
  Shader *mChunkLineShader = nullptr;
  cMeshBuffer* mChunkLineMesh = nullptr;
  Shader *mFrustumShader = nullptr;
  cMeshBuffer *mFrustumMesh = nullptr;

  // chunk load range
  Point3i mCenter;
  Vector3i mLoadRadius;
  Vector3i mChunkDim;
  Point3i mMinChunk;
  Point3i mMaxChunk;

  // fog
  float mFogStart;
  float mFogEnd;
  Vector3f mDirScale;
  bool mRadChanged = false;

  // lighting
  int mLighting = 0;

  // player/camera positions
  Point3f mCamPos;
  Point3i mPlayerStartPos;
  bool mPlayerReady = false;

  // timing update loop
  double mUpdateTime = 0.0;
  int mUpdateCount = 0;
  
  void addChunkFace(MeshData &data, Chunk *chunk, const Point3i &cp, bool meshed);

  bool isEdge(hash_t hash);
  blockSide_t getEdges(hash_t hash);
  void meshChunk(Chunk *chunk);
  
  block_t* atBlock(const Point3i &wp);
  block_t getBlock(const Point3i &wp);
  //void updateAdjacent(const Point3i &wp);

  //bool neighborsLoaded(hash_t hash, Chunk *chunk);
  void chunkLoadCallback(Chunk *chunk);
  //bool checkChunkLoad(const Point3i &cp);

  Point3i playerStartPos() const;
  int getHeightAt(const Point2i &xy, bool *success = nullptr);

  MeshData makeChunkLineMesh();
  MeshData makeFrustumMesh();
  bool updateInfo();
};

#endif // WORLD_HPP
