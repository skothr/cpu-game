#ifndef VOXEL_ENGINE_HPP
#define VOXEL_ENGINE_HPP

#include <string>

#include "input.hpp"
#include "matrix.hpp"
#include "timedThread.hpp"
#include "threadPool.hpp"
#include "world.hpp"
#include "params.hpp"
#include "tool.hpp"

#define GOD_PLAYER true

class QObject;
class TimedThread;
class World;
class Player;

class VoxelEngine
{
public:
  static const int mLightLevels = 16;
  
  VoxelEngine(QObject *qParent = nullptr);
  ~VoxelEngine();

  bool loadWorld(World::Options &opt);
  bool createWorld(World::Options &opt);
  
  void start();
  void stop();
  void pause(bool state);
  bool isPaused() const;
  bool isRunning() const;
  
  void setUpdateTimestep(int stepTimeMs);
  void setPhysicsTimestep(int stepTimeMs);
  void startPhysics();
  void stopPhysics();
  
  void setDebug(bool debug);
  void setWireframe(bool wireframe);
  
  bool initGL(QObject *qparent);
  void cleanUpGL();
  void render();

  double getFramerate() const;
  
  struct ProjDesc
  {
    float aspect = PLAYER_ASPECT;
    float fov = PLAYER_FOV;
    float znear = PLAYER_Z_NEAR;
    float zfar = PLAYER_Z_FAR;
  } mProjDesc;
  ProjDesc getProjection() const;
  void setProjection(const ProjDesc &proj);
  void setLightLevel(int lightLevel);
  
  Player* getPlayer();
  World* getWorld();
  void sendInput(const InputData &data);

  void setMaterial(block_t type, BlockData *data);
  void setTool(tool_t tool);
  void setToolRad(int rad);
  void nextTool();
  void prevTool();
  
private:
  bool mInitialized = false;
  bool mRunning = false;
  bool mPaused = false;
  bool mWireframe = false;
  bool mWireframeChanged = false;

  double mFramerate = 0.0;
  
  World *mWorld;
  Player *mPlayer;
  TimedThread mPhysicsThread;
  TimedThread mBlockThread;
  ThreadPool mMainThread;
  
  double printFps();
  void stepPhysics(int us);
  void stepBlocks(int us);
  void mainLoop();
  
};


#endif // VOXEL_ENGINE_HPP
