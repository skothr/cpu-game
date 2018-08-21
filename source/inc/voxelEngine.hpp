#ifndef VOXEL_ENGINE_HPP
#define VOXEL_ENGINE_HPP

#include "input.hpp"
#include "geometry.hpp"
#include "block.hpp"
#include <string>

class cTimedThread;
class cWorld;
class cGodPlayer;
class cFpsPlayer;
class cPlayer;
class cShader;
class cTextureAtlas;
class QObject;

class cVoxelEngine
{
public:
  cVoxelEngine(QObject *qParent = nullptr, int numThreads = 1, const std::string &worldName = "",
               uint32_t seed = 0);
  ~cVoxelEngine();

  bool initGl(QObject *qparent);   // must be called from render thread
  void cleanUpGl(); // must be called from render thread
  void render();

  // physics/timing
  void stepPhysics(int us);
  void updateBlocks(int us);

  void startPhysics(int stepTimeMs);
  void stopPhysics();
  
  struct ProjDesc
  {
    float aspect = 1.0;
    float fov = 45.0;
    float znear = 0.01;
    float zfar = 1000.0;
  } mProjDesc;

  ProjDesc getProjection() const;
  void setProjection(const ProjDesc &proj);

  void setTool(block_t type);
  void nextTool();
  void prevTool();
  void sendInput(const InputData &data);

  cPlayer* getPlayer();
  
private:
  cWorld *mWorld = nullptr;    // WORLD -- handles all physical elements in the world
  cGodPlayer *mPlayer = nullptr;  // PLAYER -- handles interface between the user and the world
  cTimedThread *mBlockUpdater = nullptr;
  cTimedThread *mPhysics = nullptr;
  
  Matrix4 mProjMat;

  double printFps() const;
};


#endif // VOXEL_ENGINE_HPP
