#include "voxelEngine.hpp"

#include "logging.hpp"
#include "world.hpp"
#include "player.hpp"
#include "timedThread.hpp"
#include "shader.hpp"
#include "textureAtlas.hpp"

#include <chrono>


#define START_CHUNK Point3i({0, 0, 0})
#define CHUNK_RAD Vector3i({2, 2, 1})
#define START_POS Point3f({START_CHUNK[0]*cChunk::sizeX + cChunk::sizeX/2, \
				  START_CHUNK[1]*cChunk::sizeY + cChunk::sizeY/2, \
				  START_CHUNK[2]*cChunk::sizeZ + 8 })
#define PLAYER_EYE Vector3f{0,1,0}
#define PLAYER_UP Vector3f{0,0,1}

#define PLAYER_FOV 45
#define PLAYER_ASPECT 1.0
#define PLAYER_Z_NEAR 0.01
#define PLAYER_Z_FAR 1000.0


#define PHYSICS_TIMESTEP_MS 10 //ms
#define PHYSICS_TIMESTEP_S (TIMESTEP_MS / 1000.0f)
#define BLOCK_TIMESTEP_MS 20 //ms
#define BLOCK_TIMESTEP_S (BLOCK_TIMESTEP_MS / 1000.0f)


cVoxelEngine::cVoxelEngine(QObject *qParent, int numThreads, const std::string &worldName, uint32_t seed)
{
  mWorld = new cWorld(worldName, numThreads, START_CHUNK, CHUNK_RAD);
  mWorld->setSeed(seed);
  mPlayer = new cGodPlayer(qParent, START_POS, PLAYER_EYE, PLAYER_UP, mWorld);
  mPhysics = new cTimedThread("Physics", std::bind(&cVoxelEngine::stepPhysics,
                                                   this, std::placeholders::_1), false );
  mSimpleShader = new cShader(qParent);
  mComplexShader = new cShader(qParent);
  mTexAtlas = new cTextureAtlas(qParent, ATLAS_BLOCK_SIZE);
}

cVoxelEngine::~cVoxelEngine()
{
  stopPhysics();
  
  delete mWorld;
  delete mPlayer;
  delete mPhysics;
}


void cVoxelEngine::startPhysics(int stepTimeMs)
{
  LOGD("PHYSICS START");
  mPhysics->setInterval(stepTimeMs * 1000);
  mPhysics->start();
}

void cVoxelEngine::stopPhysics()
{
  LOGD("PHYSICS STOP");
  mPhysics->join();
}
void cVoxelEngine::stepPhysics(int us)
{
  //LOGD("PHYSICS STEP");
  static bool ready = false;
  if(!ready)
    {
      if(mWorld->readyForPlayer())
        {
          ready = true;
          //std::cout << "Setting player position...\n";
          //mPlayer.setPos(mWorld.getStartPos(PLAYER_START_POS));
          //std::cout << "Done setting player position!\n";
        }
    }
  else
    {
      mPlayer->update((double)us / 1000000.0);
    }
}
void cVoxelEngine::updateBlocks(int us)
{
  //mWorld.update();
}

bool cVoxelEngine::initGl()
{
  LOGD("VOXEL INIT GL");
  // optimizations
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);

  
  // load shaders
  if(!mSimpleShader->loadProgram("./shaders/simpleBlock.vsh", "./shaders/simpleBlock.fsh",
				 {"posAttr", "normalAttr", "texCoordAttr"}, {"pvm", "uTex"} ))
    {
      LOGE("Simple block shader failed to load!");
      return false;
    }
  // if(!mComplexShader->loadProgram("./shaders/complexBlock.vsh", "./shaders/complexBlock.fsh",
  //       			  {"posAttr", "normalAttr", "texCoordAttr"}, {"pvm", "uBlockType", "uTex"} ))
  //   {
  //     LOGE("Complex block shader failed to load!");
  //     throw std::exception();
  //   }

  
  Matrix4 viewMat = mPlayer->getView();
  mProjMat = matProjection(PLAYER_FOV, PLAYER_ASPECT, PLAYER_Z_NEAR, PLAYER_Z_FAR);

  
  LOGD("simpleShader");
  mSimpleShader->bind();
  mSimpleShader->setUniform("pvm", mProjMat * viewMat);
  mSimpleShader->setUniform("uTex", 0);
  LOGD("mWorld initGL");
  mWorld->initGL(mSimpleShader);
  mSimpleShader->release();
  
  // LOGD("complexShader");
  // mComplexShader->bind();
  // mComplexShader->setUniform("pvm", mProjMat*viewMat);
  // mComplexShader->setUniform("uTex", 0);
  // cWorld::modelInitGL(mComplexShader);
  // mComplexShader->release();
  
  if(!mPlayer->initGL())
    {
      LOGE("Player initGl() failed!");
    }
LOGD("Tex init GL..");

std::cout << "TES ATLAS: " << (long)mTexAtlas << "\n";

  if(!mTexAtlas->create("./res/texAtlas.png"))
    {
      LOGE("Failed to load texture atlas!");
    }
  std::cout << "Starting player physics...\n";
startPhysics(PHYSICS_TIMESTEP_MS);
  std::cout << "Starting to load chunks...\n";
  mWorld->startLoading();
  std::cout << "Started" << std::endl;
  LOGD("VOXEL INIT GL DONE");
return true;
}

void cVoxelEngine::cleanUpGl()
{
  LOGD("VOXEL CLEANUP GL");
  mWorld->cleanupGL();
  mPlayer->cleanupGL();
  mTexAtlas->destroy();
  LOGD("VOXEL CLEANUP GL DONE");
}

double cVoxelEngine::printFps() const
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
      std::cout << "RENDER FRAMERATE: " << framerate << "\n";

      printCounter = 0.0;
      numIterations = 0;
      result = framerate;
    }
  lastTime = time;
  return result;
}

void cVoxelEngine::render()
{
  //LOGD("VOXEL RENDER");
  printFps();
  
  Matrix4 m = mProjMat * mPlayer->getView();
  
  //mWorld.update();

  mTexAtlas->bind();
  mSimpleShader->bind();
  mWorld->renderSimple(mSimpleShader, m);
  mSimpleShader->release();
  // mComplexShader->bind();
  // mWorld.renderComplex(mComplexShader, m);
  // mComplexShader->release();
  mTexAtlas->release();
  mPlayer->render(m);
 // LOGD("VOXEL RENDER DONE");
  //glFinish();
}


cVoxelEngine::ProjDesc cVoxelEngine::getProjection() const
{ return mProjDesc; }

void cVoxelEngine::setProjection(const ProjDesc &desc)
{
  //LOGD("SET PROJECTION");
  std::memcpy((void*)&mProjDesc, (void*)&desc, sizeof(ProjDesc));
    
  // TODO: Get this working with Matrix4 class on its own
  QMatrix4x4 p;
  p.perspective(mProjDesc.fov, mProjDesc.aspect, mProjDesc.znear, mProjDesc.zfar);
  mProjMat = p;
}


void cVoxelEngine::sendInput(const InputData &data)
{
#define PLAYER_SPEED (80.0 * (20.0f / PHYSICS_TIMESTEP_MS))

  //LOGD("SEND INPUT");
  switch(data.type)
    {
      // movement
    case input_t::MOVE_RIGHT:
      mPlayer->addXForce((data.movement.keyDown) ? PLAYER_SPEED : 0.0);
      break;
    case input_t::MOVE_LEFT:
      mPlayer->addXForce(data.movement.keyDown ? -PLAYER_SPEED : 0.0);
      break;
    case input_t::MOVE_FORWARD:
      mPlayer->addYForce(data.movement.keyDown ? PLAYER_SPEED : 0.0);
      break;
    case input_t::MOVE_BACK:
      mPlayer->addYForce(data.movement.keyDown ? -PLAYER_SPEED : 0.0);
      break;
    case input_t::MOVE_UP:
      mPlayer->addZForce(data.movement.keyDown ? PLAYER_SPEED : 0.0);
break;
    case input_t::MOVE_DOWN:
      mPlayer->addZForce(data.movement.keyDown ? -PLAYER_SPEED : 0.0);
      break;

    case input_t::ACTION_JUMP:
      //mPlayer->jump(data.action.magnitude);
      break;
    case input_t::ACTION_SNEAK:
      break;

    case input_t::MOUSE_MOVE:
      mPlayer->rotate(data.mouseMove.dPos[1]*0.08f, data.mouseMove.dPos[0]*0.06f*mProjDesc.aspect);
      break;
    case input_t::MOUSE_CLICK:
      switch(data.mouseClick.button)
        {
        case mouseButton_t::LEFT:
          LOGD("PICKING UP..");
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



cPlayer* cVoxelEngine::getPlayer()
{ return mPlayer; }
