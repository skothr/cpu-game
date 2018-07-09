#include "gameWidget.hpp"

#include "logging.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>

#include <unistd.h>
#include <exception>

#include <QString>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QCursor>
#include <functional>
#include <QWidget>

#include "shader.hpp"
#include "vertex.hpp"
#include "objLoader.hpp"
#include "block.hpp"
#include "vector.hpp"

#define VSHADER_FILE "shaders/block.vsh"
#define FSHADER_FILE "shaders/block.fsh"

#define TIMESTEP_MS 5 //ms
#define TIMESTEP_S (TIMESTEP_MS / 1000.0f)
#define RENDER_TIMESTEP_MS 7 //5 //ms
#define RENDER_TIMESTEP_S (RENDER_TIMESTEP_MS / 1000.0f)

#define CUBE_OBJ_PATH "./res/cube.obj"

block_t gBlock;


cGameWidget::cGameWidget(QWidget *parent)
  : GlWidget(parent), mChunkMap(Point2i{0,0}),//mChunk(Vector3i{CHUNK_BLOCKS, CHUNK_BLOCKS, CHUNK_BLOCKS}),
    mPlayer({3,3,1}, {0,1,0}, {0,0,1}, &this->mChunkMap), mMousePos(-1000,-1000),
    mTex(QOpenGLTexture::Target::Target2D)
{
  cChunkMap::loadModels();
  
  mChunkMap.loadChunk(Point2i{0,0});
  //mChunkMap.loadChunk(Point2i{0,1});
  //mChunkMap.loadChunk(Point2i{1,0});
  //mChunkMap.loadChunk(Point2i{1, 1});
  // floor
  for(int x = 0; x < CHUNK_BLOCKS; x++)
    {
      for(int y = 0; y < CHUNK_BLOCKS; y++)
	{
	  for(int z = 0; z < CHUNK_BLOCKS/2; z++)
	    {
	      if(x == 0 || x == CHUNK_BLOCKS-1 ||
		 y == 0 || y == CHUNK_BLOCKS-1 ||
		 z == 0 || z == CHUNK_BLOCKS/2 - 1 )
		{
		  mChunkMap.set(Point3i{x,y,z}, block_t::FLOOR);
		}
	    }
	}
    }
  
  // devices
  mChunkMap.set(Point3i{10, 10, 1}, block_t::CPU);
  mChunkMap.set(Point3i{10, 11, 1}, block_t::DEVICE);
  mChunkMap.set(Point3i{10, 12, 1}, block_t::MEMORY);
  
  mBlockShader = new cShader(this);
  mWireShader = new cShader(this);

  mPhysics = new cTimedThread(this);
  mPhysics->setInterval(TIMESTEP_MS * 1000);
  connect(mPhysics, SIGNAL(step(int)), this, SLOT(stepPhysics(int)));
  mPhysics->start();
  
  mRender = new cTimedThread(this, true);
  mRender->setInterval(RENDER_TIMESTEP_MS * 1000);
  connect(mRender, SIGNAL(step(int)), this, SLOT(update()));
  mRender->start();

  setFocus();//Qt::ActiveWindowFocusReason);
  setFocusPolicy((Qt::FocusPolicy)(Qt::StrongFocus | Qt::WheelFocus));
  setMouseTracking(true);
}

cGameWidget::~cGameWidget()
{
  LOGD("~cGameWidget()...");
  mPhysics->stop();
  while(!mPhysics->isFinished())
    { usleep(1000); }

  mRender->stop();
  while(!mRender->isFinished())
    { usleep(1000); }

  LOGD("Cleaning up GL...");
  makeCurrent();
  mChunkMap.cleanupGL();
  cChunkMap::modelCleanupGL();
  mPlayer.cleanupGL();
  mTex.destroy();
  doneCurrent();
  LOGD("Done cleaning up GL.");
}


void cGameWidget::glInit()
{
  GlWidget::glInit();

  std::vector<std::string> attrs{"posAttr", "normalAttr", "texCoordAttr"};
  std::vector<std::string> unifs{"pvm", "uBlockType", "uTex"};
  if(!mBlockShader->loadProgram(VSHADER_FILE, FSHADER_FILE, attrs,
			   unifs ))
    {
      LOGE("Block shader failed to load!");
      throw std::exception();
    }
  unifs.pop_back();
  if(!mWireShader->loadProgram("./shaders/wireframe.vsh", "./shaders/wireframe.fsh",
			       attrs, unifs))
    {
      LOGE("Wireframe shader failed to load!");
      throw std::exception();
    }

  mProjMat = matProjection(fov, aspect, zNear, zFar);
  Matrix4 viewMat = mPlayer.getView();
  mBlockShader->bind();
  mBlockShader->setUniform("pvm", mProjMat*viewMat);
  mBlockShader->setUniform("uTex", 0);
  cChunkMap::modelInitGL(mBlockShader);
  mChunkMap.initGL(mBlockShader);
  mBlockShader->release();
  
  mWireShader->bind();
  mWireShader->setUniform("pvm", mProjMat*viewMat);
  mPlayer.initGL(mWireShader);
  mWireShader->release();

  // textures
  if(!mTex.create())
    {
      LOGE("Failed to create OpenGL texture!");
    }
  else
    {
      QImage texImg("./res/texAtlas.png");
      mTex.bind(0);
      mTex.setMipLevels(4);
      mTex.setData(texImg, QOpenGLTexture::GenerateMipMaps);
      mTex.setWrapMode(QOpenGLTexture::ClampToEdge);
      mTex.setMinMagFilters(QOpenGLTexture::NearestMipMapLinear, QOpenGLTexture::Nearest);
      mTex.generateMipMaps(0);
      mTex.release();
    }
}

void cGameWidget::render()
{
  static double printCounter = 0.0;
  static int numIterations = 0;
  static auto lastTime = std::chrono::steady_clock::now();
  auto time = std::chrono::steady_clock::now();
  
  printCounter += (double)std::chrono::duration_cast<std::chrono::nanoseconds>(time - lastTime).count() / 1000000000.0;
  numIterations++;

  static float t = 0.0;
  t += 0.1;

  if(printCounter > 2.0)
    {
      double framerate = (double)numIterations / printCounter;
      std::cout << "RENDER FRAMERATE: " << framerate << "\n";

      printCounter = 0.0;
      numIterations = 0;
    }
  lastTime = time;
  
  Matrix4 m = mProjMat*mPlayer.getView();

  mBlockShader->bind();
  mTex.bind(0);
  mChunkMap.render(mBlockShader, m);
  mTex.release(0);
  mBlockShader->release();
  
  mWireShader->bind();
  mPlayer.render(mWireShader, m);
  mWireShader->release();
}


void cGameWidget::stepPhysics(int us)
{
  mPlayer.update((double)us / 1000000.0);
}
void cGameWidget::updateGraphics(int us)
{
  update();
}

void cGameWidget::mousePressEvent(QMouseEvent *event)
{
  mMouseDown = true;
  if(event->button() == Qt::LeftButton)
    {
      mPlayer.interact();
    }
  else if(event->button() == Qt::RightButton)
    {
      mPlayer.place();
    }
}
void cGameWidget::mouseReleaseEvent(QMouseEvent *event)
{
  Q_UNUSED(event);
  mMouseDown = false;
}
void cGameWidget::mouseMoveEvent(QMouseEvent *event)
{
  if(mMousePos.x() > -500)
    {
      QPoint dPos = event->pos() - mMousePos;
      mPlayer.rotate(dPos.y()*0.1f, dPos.x()*aspect*0.08f);
    }
  QPoint glob = mapToGlobal(QPoint(width()/2, height()/2));
  QCursor::setPos(glob);
  mMousePos = QPoint(width()/2, height()/2);
  QOpenGLWidget::mouseMoveEvent(event);
}

void cGameWidget::keyPressEvent(QKeyEvent *event)
{
  switch(event->key())
    {
      case Qt::Key_Escape:
	LOGI("Escape pressed (cGameWidget).");
      // close parent widget
      // TODO: Recursively close in case multiple super-parents
      event->ignore();
      return;

      // move player
    case Qt::Key_W:
      mPlayer.addForce(0.0, 40.0);
      break;
    case Qt::Key_S:
      mPlayer.addForce(0.0, -40.0);
      break;
    case Qt::Key_D:
      mPlayer.addForce(40.0, 0.0);
      break;
    case Qt::Key_A:
      mPlayer.addForce(-40.0, 0.0);
      break;


    case Qt::Key_Space:
      mPlayer.jump(0.18);
      break;
    // case Qt::Key_Z: // decrease scale (smaller -- zoom in)
    //   mScaleMult = -mkScaleForce;
    //   break;
    // case Qt::Key_X: // increase scale (bigger -- zoom out)
    //   mScaleMult = mkScaleForce;
    //   break;

    //   //Max iterations step
    // case Qt::Key_E: // increase max iterations
    //   emit iterChanged(mIterations + 1);
    //   break;
    // case Qt::Key_Q: // decrease max iterations
    //   emit iterChanged(mIterations - 1);
    //   break;

    // case Qt::Key_1: // super-sampling x1 (none)
    //   emit samplesChanged(1);
    //   break;
    // case Qt::Key_2: // super-sampling x2
    //   emit samplesChanged(2);
    //   break;
    // case Qt::Key_3: // super-sampling x4
    //   emit samplesChanged(4);
    //   break;
    // case Qt::Key_4: // super-sampling x8
    //   emit samplesChanged(8);
    //   break;
    // case Qt::Key_5: // super-sampling x16
    //   emit samplesChanged(16);
    //   break;

    // case Qt::Key_Greater: // increase disance cutoff
    //   emit distChanged(mDistCutoff + mkDistStep);
    //   break;
    // case Qt::Key_Less: // decrease distance cutoff
    //   emit distChanged(mDistCutoff - mkDistStep);
    //   break;

    default:
      break;
    }

  event->accept();
}

void cGameWidget::keyReleaseEvent(QKeyEvent *event)
{
  switch(event->key())
    {
      // stop moving player
    case Qt::Key_W:
      mPlayer.setYForce(0);
      break;
    case Qt::Key_S:
      mPlayer.setYForce(0);
      break;
    case Qt::Key_D:
      mPlayer.setXForce(0);
      break;
    case Qt::Key_A:
      mPlayer.setXForce(0);
      break;
      
    default:
      break;
    }

  event->accept();
}

void cGameWidget::wheelEvent(QWheelEvent *event)
{
  QPoint numPixels = event->pixelDelta();
  event->accept();
}

void cGameWidget::onResize(int w, int h)
{
  aspect = (float)w/(float)h;
  //mProjMat.identity();
  QMatrix4x4 p;
  p.perspective(fov, aspect, zNear, zFar);
  mProjMat = p;
}

void cGameWidget::resizeEvent(QResizeEvent *event)
{
  GlWidget::resizeEvent(event);
  static bool first = true;
  if(first)
    { first = false; }
  else
    { }
  event->accept();
}
