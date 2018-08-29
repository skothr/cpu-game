#include "voxelEngine.hpp"

#include "logging.hpp"

#include <chrono>


#define START_CHUNK Point3i({0, 0, 0})
#define CHUNK_RAD Vector3i({8, 8, 2})
#define START_POS Point3f({START_CHUNK[0]*Chunk::sizeX + Chunk::sizeX/2, \
                           START_CHUNK[1]*Chunk::sizeY + Chunk::sizeY/2, \
                           START_CHUNK[2]*Chunk::sizeZ + 8 })
#define PLAYER_EYE Vector3f{0,1,0}
#define PLAYER_UP Vector3f{0,0,1}

#define PLAYER_FOV 70
#define PLAYER_ASPECT 1.0
#define PLAYER_Z_NEAR 0.2
#define PLAYER_Z_FAR 512.0

#define PHYSICS_TIMESTEP_MS 10 //ms
#define PHYSICS_TIMESTEP_S (TIMESTEP_MS / 1000.0f)
#define BLOCK_TIMESTEP_MS 20 //ms
#define BLOCK_TIMESTEP_US (BLOCK_TIMESTEP_MS * 1000)


VoxelEngine::VoxelEngine(QObject *qParent, int numThreads, const std::string &worldName, uint32_t seed)
  : mWorld(numThreads, START_CHUNK, CHUNK_RAD),
    mPlayer(qParent, START_POS, PLAYER_EYE, PLAYER_UP, &mWorld),
    mMainThread(1, std::bind(&VoxelEngine::mainLoop, this), 1000),
    mPhysicsThread("Physics", std::bind(&VoxelEngine::stepPhysics, this, std::placeholders::_1), false)
{
  if(!mWorld.loadWorld(worldName, seed))
    {
      LOGE("WORLD FAILED TO LOAD FILE!!");
      exit(1);
    }
}

VoxelEngine::~VoxelEngine()
{ stop(); }

void VoxelEngine::start()
{
  mWorld.startLoading();
  mMainThread.start();
}
void VoxelEngine::stop()
{
  stopPhysics();
  mMainThread.stop();
}
void VoxelEngine::mainLoop()
{
  mWorld.update();
}

void VoxelEngine::startPhysics(int stepTimeMs)
{
  mPhysicsThread.setInterval(stepTimeMs * 1000);
  mPhysicsThread.start();
  LOGD("PHYSICS STARTED");
}
void VoxelEngine::stopPhysics()
{
  mPhysicsThread.join();
  LOGD("PHYSICS STOPPED");
}

void VoxelEngine::stepPhysics(int us)
{
  static bool ready = true; // false;
  if(!ready)
    {
      if(mWorld.readyForPlayer())
        {
          ready = true;
          //mPlayer.setPos(mWorld.getStartPos(PLAYER_START_POS));
        }
    }
  else
    { mPlayer.update((double)us / 1000000.0); }
}

bool VoxelEngine::initGL(QObject *qparent)
{
  LOGD("VOXEL INIT GL");
  
  // OpenGL configuration
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);

  // matrices
  Matrix4 viewMat = mPlayer.getView();
  mProjMat = matProjection(PLAYER_FOV, PLAYER_ASPECT, PLAYER_Z_NEAR, PLAYER_Z_FAR);

  // initialize
  mWorld.initGL(qparent);
  if(!mPlayer.initGL())
    {
      LOGE("Player initGL() failed!");
      exit(1);
    }

  // start engine
  LOGD("Starting player physics...");
  startPhysics(PHYSICS_TIMESTEP_MS);
  LOGD("Starting main thread...");
  start();
  LOGD("VOXEL INIT GL DONE");
  
  return true;
}

void VoxelEngine::cleanUpGL()
{
  LOGD("VOXEL CLEANUP GL");
  mWorld.cleanupGL();
  mPlayer.cleanupGL();
  LOGD("VOXEL CLEANUP GL DONE");
}

double VoxelEngine::printFps() const
{
  static double printCounter = 0.0;
  static int numIterations = 0;
  static auto lastTime = std::chrono::steady_clock::now();
  auto time = std::chrono::steady_clock::now();

  double result = 0.0;
  double dt = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(time - lastTime).count() / 1000000000.0;
  printCounter += dt;
  numIterations++;

  static float t = 0.0;
  t += 0.1;

  if(printCounter > 2.0)
    {
      double framerate = (double)numIterations / printCounter;
      LOGI("RENDER FRAMERATE: %f", framerate);

      printCounter = 0.0;
      numIterations = 0;
      result = framerate;
    }
  lastTime = time;
  return result;
}

void VoxelEngine::render()
{
  if(mWireframeChanged)
    {
      mWireframeChanged = false;
      if(mWireframe)
        { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }
      else
        { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }
    }
  Matrix4 m = mProjMat * mPlayer.getView();
  mWorld.setCamPos(mPlayer.getPos());
  mWorld.render(m);
  mPlayer.render(m);
  //glFinish();
  printFps();
}

void VoxelEngine::setWireframe(bool wireframe)
{
  bool changed = mWireframe != wireframe;
  mWireframe = wireframe;
  if(changed)
    { mWireframeChanged = true; }
}


Player* VoxelEngine::getPlayer()
{ return &mPlayer; }

VoxelEngine::ProjDesc VoxelEngine::getProjection() const
{ return mProjDesc; }

void VoxelEngine::setProjection(const ProjDesc &desc)
{
  std::memcpy((void*)&mProjDesc, (void*)&desc, sizeof(ProjDesc));
  
  QMatrix4x4 p;
  p.perspective(mProjDesc.fov, mProjDesc.aspect, mProjDesc.znear, mProjDesc.zfar);
  mProjMat = p;
}

void VoxelEngine::setLightLevel(int lightLevel)
{ mWorld.setLightLevel(lightLevel); }

void VoxelEngine::setTool(block_t type)
{ mPlayer.setPlaceBlock(type); }
void VoxelEngine::nextTool()
{ mPlayer.nextPlaceBlock(); }
void VoxelEngine::prevTool()
{ mPlayer.prevPlaceBlock(); }


#define PLAYER_SPEED (80.0 * (20.0f / PHYSICS_TIMESTEP_MS))
void VoxelEngine::sendInput(const InputData &data)
{
  switch(data.type)
    {
      // movement
    case input_t::MOVE_RIGHT:
      mPlayer.addXForce((data.movement.keyDown) ? PLAYER_SPEED : 0.0);
      break;
    case input_t::MOVE_LEFT:
      mPlayer.addXForce(data.movement.keyDown ? -PLAYER_SPEED : 0.0);
      break;
    case input_t::MOVE_FORWARD:
      mPlayer.addYForce(data.movement.keyDown ? PLAYER_SPEED : 0.0);
      break;
    case input_t::MOVE_BACK:
      mPlayer.addYForce(data.movement.keyDown ? -PLAYER_SPEED : 0.0);
      break;
    case input_t::MOVE_UP:
      mPlayer.addZForce(data.movement.keyDown ? PLAYER_SPEED : 0.0);
break;
    case input_t::MOVE_DOWN:
      mPlayer.addZForce(data.movement.keyDown ? -PLAYER_SPEED : 0.0);
      break;

    case input_t::ACTION_JUMP: // (only for fps player)
      //mPlayer.jump(data.action.magnitude);
      break;
    case input_t::ACTION_SNEAK:
      break;

    case input_t::MOUSE_MOVE:
      if(data.mouseMove.captured)
        {
          mPlayer.setSelectMode(false);
          mPlayer.rotate(data.mouseMove.dPos[1]*0.08f, -data.mouseMove.dPos[0]*0.06f*mProjDesc.aspect);
        }
      else
        {
          mPlayer.setSelectMode(true);
          mPlayer.select(data.mouseMove.vPos * 2.0f - 1.0f, mProjMat);
        }
      break;
    case input_t::MOUSE_CLICK:
      switch(data.mouseClick.button)
        {
        case mouseButton_t::LEFT:
          mPlayer.pickUp();
          break;
        case mouseButton_t::RIGHT:
          mPlayer.place();
          break;
        case mouseButton_t::MIDDLE:
          break;
        }
    }
}
