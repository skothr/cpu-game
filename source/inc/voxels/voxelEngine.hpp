#ifndef VOXEL_ENGINE_HPP
#define VOXEL_ENGINE_HPP

#include <string>

#include "input.hpp"
#include "geometry.hpp"
#include "timedThread.hpp"
#include "player.hpp"
#include "world.hpp"

class QObject;
class TimedThread;
class GodPlayer;
class FpsPlayer;

class VoxelEngine
{
public:
  static const int mLightLevels = 16;
  
  VoxelEngine(QObject *qParent = nullptr, int numThreads = 1, const std::string &worldName = "",
               uint32_t seed = 0);
  ~VoxelEngine();

  void start();
  void stop();
  void startPhysics(int stepTimeMs);
  void stopPhysics();

  void stepPhysics(int us);
  void stepBlocks(int us);
  
  bool initGL(QObject *qparent);
  void cleanUpGL();
  void render();

  void setDebug(bool debug)
  {
    mWorld.setDebug(debug);
  }

  void setWireframe(bool wireframe);
  
  struct ProjDesc
  {
    float aspect = 1.0;
    float fov = 45.0;
    float znear = 0.01;
    float zfar = 1000.0;
  } mProjDesc;

  Player* getPlayer();
  World* getWorld();
  ProjDesc getProjection() const;
  void setProjection(const ProjDesc &proj);
  void setLightLevel(int lightLevel);

  void setTool(block_t type, BlockData *data);
  void nextTool();
  void prevTool();
  void sendInput(const InputData &data);
  
private:
  World mWorld;
  GodPlayer mPlayer;
  TimedThread mPhysicsThread;
  TimedThread mBlockThread;
  ThreadPool mMainThread;
  Matrix4 mProjMat;
  bool mWireframe = false;
  bool mWireframeChanged = false;

  double printFps() const;
  void mainLoop();
};


#endif // VOXEL_ENGINE_HPP
