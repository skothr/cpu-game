#include "voxelEngine.hpp"

#include "logging.hpp"
#include "player.hpp"
#include <chrono>

VoxelEngine::VoxelEngine(QObject *qParent)
  : mMainThread(1, std::bind(&VoxelEngine::mainLoop, this), MAIN_THREAD_SLEEP_MS*1000),
    mBlockThread("Block Update", std::bind(&VoxelEngine::stepBlocks, this, std::placeholders::_1), false),
    mPhysicsThread("Physics", std::bind(&VoxelEngine::stepPhysics, this, std::placeholders::_1), false),
    mWorld(new World()), mPlayer(new Player(Point3f{0,0,0}, PLAYER_EYE, PLAYER_UP, mWorld))
{
  mWorld->setCamera(mPlayer->getCamera());
}

VoxelEngine::~VoxelEngine()
{
  stop();
  delete mPlayer;
  delete mWorld;
}

bool VoxelEngine::loadWorld(World::Options &opt)
{
  return mWorld->loadWorld(opt);
}
bool VoxelEngine::createWorld(World::Options &opt)
{
  return mWorld->createWorld(opt);
}

void VoxelEngine::start()
{
  if(!mRunning)
    {
      LOGI("Starting voxel engine...");
      LOGI("  Starting world...");
      mWorld->start();
      LOGI("  Starting main thread...");
      mMainThread.start();
      LOGI("  Starting block update thread...");
      setUpdateTimestep(BLOCK_TIMESTEP_MS);
      mBlockThread.start();
      LOGI("  Starting physics thread...");
      setPhysicsTimestep(PHYSICS_TIMESTEP_MS);
      startPhysics();
      LOGI("Voxel Engine started.");
      mRunning = true;
    }
}
void VoxelEngine::stop()
{
  if(mRunning)
    {
      mWorld->setPlayerPos(mPlayer->getPos());
      LOGI("Stopping voxel engine...");
      LOGI("  Stopping physics...");
      stopPhysics();
      LOGI("  Stopping block thread..");
      mBlockThread.join();
      LOGI("  Stopping main thread...");
      mMainThread.stop();
      LOGI("  Stopping world...");
      mWorld->stop();
      LOGI("Voxel engine Stopped.");
      mRunning = false;
    }
}

void VoxelEngine::pause(bool state)
{
  mPaused = state;
}
bool VoxelEngine::isPaused() const
{
  return mPaused;
}
bool VoxelEngine::isRunning() const
{
  return mRunning;
}

void VoxelEngine::setUpdateTimestep(int stepTimeMs)
{
  mBlockThread.setInterval(stepTimeMs * 1000);
}
void VoxelEngine::setPhysicsTimestep(int stepTimeMs)
{
  mPhysicsThread.setInterval(stepTimeMs * 1000);
}
void VoxelEngine::startPhysics()
{
  mPhysicsThread.start();
}
void VoxelEngine::stopPhysics()
{
  mPhysicsThread.join();
}

void VoxelEngine::mainLoop()
{
  mWorld->update();
}
void VoxelEngine::stepBlocks(int us)
{
  if(!mPaused)
    { mWorld->step(); }
}
void VoxelEngine::stepPhysics(int us)
{
  static bool ready = false;
  if(!ready)
    {
      if(mWorld->readyForPlayer())
        {
          ready = true;
          mPlayer->setPos(mWorld->getStartPos());
        }
      else
        {
          Point3f ppos = mWorld->getStartPos();
          mPlayer->setPos(Point3f{ppos[0], ppos[1], 100.0f});
        }
    }
  else if(!mPaused)
    {
      mPlayer->update((double)us / 1000000.0);
    }
}

bool VoxelEngine::initGL(QObject *qParent)
{
  if(!mInitialized)
    {
      LOGI("Initializing voxel engine GL...");
      // OpenGL configuration
      glFrontFace(GL_CCW);
      glCullFace(GL_BACK);
      glEnable(GL_CULL_FACE);

      // initialize
      LOGI("  Initializing world GL...");
      if(!mWorld->initGL(qParent))
        {
          LOGE("  World GL initialization falied! Exiting...");
          exit(1);
        }
      LOGI("  Initializing player GL...");
      if(!mPlayer->initGL(qParent))
        {
          LOGE("  Player GL initialization failed! Exiting...");
          exit(1);
        }

      LOGI("Voxel engine GL initialized.");
      mInitialized = true;
    }
  return true;
}

void VoxelEngine::cleanUpGL()
{
  if(mInitialized)
    {
      LOGI("Uninitializing voxel engine GL...");
      LOGI("  Uninitializing player GL...");
      mPlayer->cleanupGL();
      LOGI("  Uninitializing world GL...");
      mWorld->cleanupGL();
      LOGI("Voxel engine GL uninitialized.");
      mInitialized = false;
    }
}

double VoxelEngine::getFramerate() const
{
  return mFramerate;
}


#define LIMIT_FPS false
double VoxelEngine::printFps()
{
  if(mInitialized)
    {
      static double printCounter = 0.0;
      static int numIterations = 0;
      static auto lastTime = std::chrono::steady_clock::now();
      auto time = std::chrono::steady_clock::now();

      double result = 0.0;

      double micro = std::chrono::duration_cast<std::chrono::nanoseconds>(time - lastTime).count() / 1000.0;
      double dt = micro / 1000000.0;
      printCounter += dt;
      numIterations++;

      static float t = 0.0;
      t += 0.1;

      if(printCounter > 0.1)
        {
          mFramerate = (double)numIterations / printCounter;
          printCounter = 0.0;
          numIterations = 0;
        }
      lastTime = time;
      return mFramerate;
    }
  else
    { return 0.0f; }
}

void VoxelEngine::render()
{
  if(mInitialized)
    {
      if(mWireframeChanged)
        {
          mWireframeChanged = false;
          if(mWireframe)
            { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }
          else
            { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }
        }
      Matrix4 m = mPlayer->getCamera()->getProjection() * mPlayer->getCamera()->getView();
      mWorld->setCamPos(mPlayer->getCamera()->getPos());
      mWorld->render(m);
      mPlayer->render(m);
      glFinish();
      printFps();
    }
}

void VoxelEngine::setDebug(bool debug)
{
  mWorld->setDebug(debug);
}
void VoxelEngine::setWireframe(bool wireframe)
{
  bool changed = mWireframe != wireframe;
  mWireframe = wireframe;
  if(changed)
    { mWireframeChanged = true; }
}

Player* VoxelEngine::getPlayer()
{ return mPlayer; }
World* VoxelEngine::getWorld()
{ return mWorld; }

VoxelEngine::ProjDesc VoxelEngine::getProjection() const
{
  return mProjDesc;
}

void VoxelEngine::setProjection(const ProjDesc &desc)
{
  mPlayer->getCamera()->setProjection(desc.fov, desc.aspect, desc.znear, desc.zfar);
}

void VoxelEngine::setLightLevel(int lightLevel)
{ mWorld->setLightLevel(lightLevel); }

void VoxelEngine::setMaterial(block_t type, BlockData *data)
{
  LOGD("PLAYER SETTING TOOL: %s", toString(type).c_str());
  mPlayer->setPlaceBlock(type, data);
}
void VoxelEngine::setTool(tool_t tool)
{
  mPlayer->setTool(tool);
}
void VoxelEngine::setToolRad(int rad)
{
  mPlayer->setToolRad(rad);
}
void VoxelEngine::nextTool()
{ mPlayer->nextPlaceBlock(); }
void VoxelEngine::prevTool()
{ mPlayer->prevPlaceBlock(); }

void VoxelEngine::sendInput(const InputData &data)
{
  if(mInitialized)
    {
      switch(data.type)
        {
          // movement
        case input_t::MOVE_RIGHT:
          mPlayer->moveX((data.movement.keyDown && !mPaused) ? 1.0f : 0.0f);
          break;
        case input_t::MOVE_LEFT:
          mPlayer->moveX((data.movement.keyDown && !mPaused) ? -1.0f : 0.0f);
          break;
        case input_t::MOVE_FORWARD:
          mPlayer->moveY((data.movement.keyDown && !mPaused) ? 1.0f : 0.0f);
          break;
        case input_t::MOVE_BACK:
          mPlayer->moveY((data.movement.keyDown && !mPaused) ? -1.0f : 0.0f);
          break;
        case input_t::MOVE_UP:
          mPlayer->moveZ((data.movement.keyDown && !mPaused) ? 1.0f : 0.0f);
          break;
        case input_t::MOVE_DOWN:
          mPlayer->moveZ((data.movement.keyDown && !mPaused) ? -1.0f : 0.0f);
          break;

        case input_t::ACTION_JUMP:
          if(!mPaused)
            { mPlayer->jump(data.action.keyDown); }
          break;
        case input_t::ACTION_SNEAK:
          if(!mPaused)
            { mPlayer->sneak(data.action.keyDown); }
          break;
        case input_t::ACTION_RUN:
          if(!mPaused)
            { mPlayer->run(data.action.keyDown); }
          break;

        case input_t::MOUSE_MOVE:
          if(!mPaused)
            {
              if(data.mouseMove.captured)
                {
                  mPlayer->setSelectMode(false);
                  mPlayer->getCamera()->rotate(data.mouseMove.dPos[1]*0.08f,
                                               -data.mouseMove.dPos[0]*0.06f*mProjDesc.aspect );
                }
              else
                {
                  mPlayer->setSelectMode(true);
                  mPlayer->select(data.mouseMove.vPos * 2.0f - 1.0f,
                                  mPlayer->getCamera()->getProjection() );
                }
            }
          break;
        case input_t::MOUSE_CLICK:
          if(!mPaused)
            {
              switch(data.mouseClick.button)
                {
                case mouseButton_t::LEFT:
                  mPlayer->pickUp();
                  break;
                case mouseButton_t::RIGHT:
                  mPlayer->place();
                  break;
                case mouseButton_t::MIDDLE:
                  break;
                }
            }
          
        }
    }
}
