#ifndef VOXEL_ENGINE_HPP
#define VOXEL_ENGINE_HPP

#include <string>

#include "input.hpp"
#include "geometry.hpp"
#include "timedThread.hpp"
#include "world.hpp"
#include "params.hpp"

#define GOD_PLAYER true

class QObject;
class TimedThread;
class World;

class Player;
#if GOD_PLAYER
class GodPlayer;
#else
class FpsPlayer;
#endif // GOD_PLAYER

class VoxelEngine
{
public:
  static const int mLightLevels = 16;
  
  VoxelEngine(QObject *qParent = nullptr);
  ~VoxelEngine();

  bool loadWorld(const World::Options &opt);
  bool createWorld(const World::Options &opt);
  
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

  void setTool(block_t type, BlockData *data);
  void nextTool();
  void prevTool();
  
private:
  bool mInitialized = false;
  bool mRunning = false;
  bool mPaused = false;
  bool mWireframe = false;
  bool mWireframeChanged = false;
  
#if GOD_PLAYER
  GodPlayer *mPlayer;
#else // !GOD_PLAYER (FPS_PLAYER)
  FpsPlayer *mPlayer;
#endif // GOD_PLAYER
  
  World *mWorld;
  TimedThread mPhysicsThread;
  TimedThread mBlockThread;
  ThreadPool mMainThread;
  
  double printFps() const;
  void stepPhysics(int us);
  void stepBlocks(int us);
  void mainLoop();
  
};


#endif // VOXEL_ENGINE_HPP
