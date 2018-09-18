#include "gameWidget.hpp"

#include "logging.hpp"
#include "fluid.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>

#include <unistd.h>
#include <exception>
#include <functional>

#include <QLabel>
#include <QString>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QCursor>
#include <QWidget>
#include <QTimer>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QStackedLayout>
#include <QGridLayout>

#include "block.hpp"
#include "player.hpp"
#include "vector.hpp"
#include "world.hpp"
#include "overlay.hpp"
#include "pauseWidget.hpp"
#include "controlInterface.hpp"

GameWidget::GameWidget(QWidget *parent)
  : QOpenGLWidget(parent), mMousePos(-1000,-1000)
{
  mEngine = new VoxelEngine(this);
  mRenderTimer = new QTimer(this);
  mRenderTimer->setInterval(RENDER_THREAD_SLEEP_MS);
  mInfoTimer = new QTimer(this);
  mInfoTimer->setInterval(INFO_THREAD_SLEEP_MS);

  // set opengl format
  QSurfaceFormat format;
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  format.setVersion(4, 6);
  format.setSamples(1);
  format.setRenderableType(QSurfaceFormat::OpenGL);
  setFormat(format);

  // HUD overlay
  mPlayerOverlay = new Overlay(this, {LabelDesc("POSITION", (align_t)(align_t::TOP)),
                                      LabelDesc("CHUNK", (align_t)(align_t::TOP)),
                                      LabelDesc("BLOCK", (align_t)(align_t::TOP)),
                                      LabelDesc("LIGHT", (align_t)(align_t::TOP))});
  mPlayerOverlay->raise();
  mChunkOverlay = new Overlay(this,
                              { LabelDesc("FRAMERATE", (align_t)(align_t::TOP | align_t::RIGHT)),
                                LabelDesc("CHUNKS LOADED", (align_t)(align_t::TOP | align_t::RIGHT)),
                                LabelDesc("CHUNKS MESHED", (align_t)(align_t::TOP | align_t::RIGHT)),
                                LabelDesc("BLOCKS", (align_t)(align_t::TOP | align_t::RIGHT)) });
  mChunkOverlay->raise();

  mPause = new PauseWidget(this);
  mPause->hide();
  
  // game viewport layout
  mOverlayLayout = new QStackedLayout();
  mOverlayLayout->setStackingMode(QStackedLayout::StackAll);
  mOverlayLayout->addWidget(mPlayerOverlay);
  mOverlayLayout->addWidget(mChunkOverlay);
  mOverlayLayout->addWidget(mPause);
  
  // control layouts
  mControl = new ControlInterface(mEngine);
  // add to main grid layout
  mMainLayout = new QGridLayout(this);
  mMainLayout->setSpacing(0);
  mMainLayout->setMargin(0);
  mMainLayout->addLayout(mOverlayLayout, 0, 0, 4, 4);
  mMainLayout->addWidget(mControl, 2, 0, 2, 4, Qt::AlignBottom);

  connect(mRenderTimer, SIGNAL(timeout()), this, SLOT(update())); 
  connect(mInfoTimer, SIGNAL(timeout()), SLOT(updateInfo())); 
  connect(mPause, SIGNAL(resumed()), this, SLOT(resume()));
  connect(mPause, SIGNAL(mainMenu()), this, SLOT(endGame()));
  connect(mPause, SIGNAL(quit()), this, SIGNAL(quit()));
  
  setLayout(mMainLayout);
  setFocusPolicy((Qt::FocusPolicy)(Qt::StrongFocus));
  setFocus();
  setMouseTracking(true);
}

GameWidget::~GameWidget()
{
  stop();
  makeCurrent();
  mEngine->cleanUpGL();
  doneCurrent();
  LOGD("Deleting engine...");
  delete mEngine;
  LOGD("DONE Deleting engine...");
}

VoxelEngine* GameWidget::getEngine()
{ return mEngine; }

void GameWidget::initializeGL()
{
  LOGD("INIT GL!!");
  glClearColor(0.229, 0.657, 0.921, 1.0);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  
  // init game objects
  mEngine->initGL(this);
}

void GameWidget::paintGL()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  render();
}


void GameWidget::resizeGL(int w, int h)
{
  glViewport(0, 0, w, h);

  mEngine->getWorld()->setScreenSize(Point2i{w, h});
  
  VoxelEngine::ProjDesc proj = mEngine->getProjection();
  proj.aspect = (float)w/(float)h;
  mEngine->setProjection(proj);
}

void GameWidget::render()
{
  mEngine->render();
}

void GameWidget::start()
{
  mControl->initRadius(mEngine->getWorld()->getRadius());
  mEngine->start();
  mRenderTimer->start();
  mInfoTimer->start();
}
void GameWidget::stop()
{
  mInfoTimer->stop();
  mRenderTimer->stop();
  mEngine->stop();
}
void GameWidget::pause(bool status)
{
  mEngine->pause(status);

  if(status)
    { mPause->show(); }
  else
    { mPause->hide(); }
}

void GameWidget::resume()
{
  pause(false);
}

void GameWidget::endGame()
{
  stop();
  mEngine->getWorld()->clear();
  emit mainMenu();
}
  
void GameWidget::updateInfo()
{
  // update player info
  Point3f ppos = mEngine->getPlayer()->getPos();
  Point3i coll = mEngine->getPlayer()->getCollisions();
  Point3i cpos = World::chunkPos(ppos);
  QLabel *pl = mPlayerOverlay->getLabel(0);
  QLabel *cl = mPlayerOverlay->getLabel(1);
  
  std::stringstream ss;
  ss << "Position: " << ppos << "  (" << coll << ")";
  pl->setText(ss.str().c_str());
  ss.str(""); ss.clear();
  ss << "Chunk:    " << cpos;
  cl->setText(ss.str().c_str());
  ss.str(""); ss.clear();
  
  QLabel *tl = mPlayerOverlay->getLabel(2);
  QLabel *ll = mPlayerOverlay->getLabel(3);
  CompleteBlock b = mEngine->getPlayer()->selectedBlock();
  float lightLevel = 0.0f;
  if(isFluidBlock(b.type))
    { lightLevel = reinterpret_cast<Fluid*>(b.data)->level; }
  ss << "Level:    " << lightLevel;
  ll->setText(ss.str().c_str());
  tl->setText(("Type:     " + toString(b.type)).c_str());
  ss.str(""); ss.clear();
  
  // update chunk info
  int totalChunks = mEngine->getWorld()->totalChunks();
  int centerChunks = mEngine->getWorld()->centerChunks();
  int chunksLoaded = mEngine->getWorld()->numLoaded();
  int chunksMeshed = mEngine->getWorld()->numMeshed();
  double fps = mEngine->getFramerate();

  Point3i rad = mEngine->getWorld()->getRadius() * 2 + 1;
  rad *= Chunk::size;
  
  QLabel *fpsl = mChunkOverlay->getLabel(0);
  QLabel *cll = mChunkOverlay->getLabel(1);
  QLabel *cml = mChunkOverlay->getLabel(2);
  QLabel *bl = mChunkOverlay->getLabel(3);
  ss << fps << " FPS";
  fpsl->setText(ss.str().c_str());
  ss.str(""); ss.clear();
  ss << "Loaded:  " << chunksLoaded << " / " << totalChunks;
  cll->setText(ss.str().c_str());
  ss.str(""); ss.clear();
  ss << "Meshed:  " << chunksMeshed << " / " << centerChunks;
  cml->setText(ss.str().c_str());
  ss.str(""); ss.clear();
  ss << "Blocks:  " << (rad[0]*rad[1]*rad[2]);
  bl->setText(ss.str().c_str());
}

void GameWidget::setTool(block_t type)
{
  mEngine->setTool(type, nullptr);
}

void GameWidget::captureMouse(bool capture)
{
  mMouseCaptured = capture;
}
bool GameWidget::getMouseCaptured() const
{ return mMouseCaptured; }

void GameWidget::mousePressEvent(QMouseEvent *event)
{
  mMouseDown = true;
  
  InputData data;
  data.type = input_t::MOUSE_CLICK;
  data.mouseClick.buttonDown = true;
  if(event->button() == Qt::LeftButton)
    { data.mouseClick.button = mouseButton_t::LEFT; }
  else if(event->button() == Qt::RightButton)
    { data.mouseClick.button = mouseButton_t::RIGHT; }
  else if(event->button() == Qt::MiddleButton)
    { data.mouseClick.button = mouseButton_t::MIDDLE; }
  mEngine->sendInput(data);
}
void GameWidget::mouseReleaseEvent(QMouseEvent *event)
{
  mMouseDown = false;
  
  InputData data;
  data.type = input_t::MOUSE_CLICK;
  data.mouseClick.buttonDown = false;
  if(event->button() == Qt::LeftButton)
    { data.mouseClick.button |= mouseButton_t::LEFT; }
  else if(event->button() == Qt::RightButton)
    { data.mouseClick.button |= mouseButton_t::RIGHT; }
  else if(event->button() == Qt::MiddleButton)
    { data.mouseClick.button |= mouseButton_t::MIDDLE; }
  mEngine->sendInput(data);
}
void GameWidget::mouseMoveEvent(QMouseEvent *event)
{
  if(mMousePos.x() > -500)
    {
      QPoint dPos = event->pos() - mMousePos;
      
      InputData data;      
      data.type = input_t::MOUSE_MOVE;
      data.mouseMove.vPos = Vector2f{(float)event->pos().x() / width(),
                                     (float)event->pos().y() / height() };
      data.mouseMove.dPos = Vector2i{dPos.x(), dPos.y()};
      data.mouseMove.drag = mMouseDown;
      data.mouseMove.captured = mMouseCaptured;
      mEngine->sendInput(data);
    }

  if(mMouseCaptured)
    {
      QPoint glob = mapToGlobal(QPoint(width()/2, height()/2));
      QCursor::setPos(glob);
      mMousePos = QPoint(width()/2, height()/2);
    }
  else
    {
      mMousePos = event->pos();
    }
  QOpenGLWidget::mouseMoveEvent(event);
}

void GameWidget::keyPressEvent(QKeyEvent *event)
{
  InputData data;
  switch(event->key())
    {
    case Qt::Key_Escape:   // pause game, or back up one menu
      pause(!mEngine->isPaused());
      event->accept();
      break;
    case Qt::Key_X:        // toggle mouse capture
      captureMouse(!getMouseCaptured());
      if(getMouseCaptured())
        {
          LOGD("Captured mouse!");
          mControl->collapse();
        }
      else
        {
          LOGD("Released mouse!");
          mControl->expand();
        }
      event->accept();
      break;
      
      // move player
    case Qt::Key_W:
      data.type = input_t::MOVE_FORWARD;
      data.movement.magnitude = 1.0;
      data.movement.keyDown = true;
      break;
    case Qt::Key_S:
      data.type = input_t::MOVE_BACK;
      data.movement.magnitude = 1.0;
      data.movement.keyDown = true;
      break;
    case Qt::Key_D:
      data.type = input_t::MOVE_RIGHT;
      data.movement.magnitude = 1.0;
      data.movement.keyDown = true;
      break;
    case Qt::Key_A:
      data.type = input_t::MOVE_LEFT;
      data.movement.magnitude = 1.0;
      data.movement.keyDown = true;
      break;
    case Qt::Key_E:
      data.type = input_t::MOVE_UP;
      data.movement.magnitude = 1.0;
      data.movement.keyDown = true;
      break;
    case Qt::Key_R:
      data.type = input_t::MOVE_DOWN;
      data.movement.magnitude = 1.0;
      data.movement.keyDown = true;
      break;

    case Qt::Key_Space:
      data.type = input_t::ACTION_JUMP;
      data.action.magnitude = 0.18;
      break;
    case Qt::Key_Control:
      data.type = input_t::ACTION_SNEAK;
      data.action.magnitude = 1.0;
      break;

    case Qt::Key_P:
      mEngine->getWorld()->setFrustumPause();
      break;
      
    default:
      event->ignore();
      return;
    }
  
  mEngine->sendInput(data);
  event->accept();
}

void GameWidget::keyReleaseEvent(QKeyEvent *event)
{
  InputData data;
  switch(event->key())
    {
      // stop moving player
    case Qt::Key_W:
      data.type = input_t::MOVE_FORWARD;
      data.movement.magnitude = 1.0;
      data.movement.keyDown = false;
      break;
    case Qt::Key_S:
      data.type = input_t::MOVE_BACK;
      data.movement.magnitude = 1.0;
      data.movement.keyDown = false;
      break;
    case Qt::Key_D:
      data.type = input_t::MOVE_RIGHT;
      data.movement.magnitude = 1.0;
      data.movement.keyDown = false;
      break;
    case Qt::Key_A:
      data.type = input_t::MOVE_LEFT;
      data.movement.magnitude = 1.0;
      data.movement.keyDown = false;
      break;
    case Qt::Key_E:
      data.type = input_t::MOVE_UP;
      data.movement.magnitude = 1.0;
      data.movement.keyDown = false;
      break;
    case Qt::Key_R:
      data.type = input_t::MOVE_DOWN;
      data.movement.magnitude = 1.0;
      data.movement.keyDown = false;
      break;
      
    default:
      break;
    }

  mEngine->sendInput(data);
  event->accept();
}

void GameWidget::wheelEvent(QWheelEvent *event)
{
  
  // static const int inertia = 80;
  // static int position = 0;
  // QPoint numPixels = event->pixelDelta();

  // position += numPixels.y();
  // while(position > inertia)
  //   {
  //     mEngine->nextTool();
  //     position -= inertia;
  //   }
  // while(position < -inertia)
  //   {
  //     mEngine->prevTool();
  //     position += inertia;
  //   }
  
  // event->accept();
}


/*
void GameWidget::onResize(int w, int h)
{
  VoxelEngine::ProjDesc proj = mEngine->getProjection();
  proj.aspect = (float)w/(float)h;
  mEngine->setProjection(proj);
}

void GameWidget::resizeEvent(QResizeEvent *event)
{
  GlWidget::resizeEvent(event);
  static bool first = true;
  if(first)
    { first = false; }
  else
    { }
  event->accept();
}
*/
