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
  QSurfaceFormat f = format();
  f.setSwapInterval(1);
  f.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  f.setDepthBufferSize(24);
  //f.setStencilBufferSize(8);
  f.setVersion(4, 6);
  //f.setSampleBuffers(true);
  //f.setSamples(1);
  f.setRenderableType(QSurfaceFormat::OpenGL);
  f.setProfile(QSurfaceFormat::CoreProfile);
  setFormat(f);

  // HUD overlay
  mPlayerOverlay = new Overlay(this, {LabelDesc("FRAMERATE", (align_t)(align_t::TOP | align_t::RIGHT)),
                                      LabelDesc("POSITION", (align_t)(align_t::TOP | align_t::RIGHT)),
                                      LabelDesc("CHUNK", (align_t)(align_t::TOP | align_t::RIGHT)),
                                      LabelDesc("BLOCK", (align_t)(align_t::TOP | align_t::RIGHT)),
                                      LabelDesc("LIGHT", (align_t)(align_t::TOP | align_t::RIGHT))});
  mPlayerOverlay->raise();

  mPause = new PauseWidget(this);
  mPause->hide();
  
  // game viewport layout
  mOverlayLayout = new QStackedLayout();
  mOverlayLayout->setStackingMode(QStackedLayout::StackAll);
  mOverlayLayout->addWidget(mPlayerOverlay);
  mOverlayLayout->addWidget(mPause);
  
  // control layouts
  mControl = new ControlInterface(mEngine);
  // add to main grid layout
  mMainLayout = new QGridLayout(this);
  mMainLayout->setSpacing(0);
  mMainLayout->setMargin(0);
  mMainLayout->addLayout(mOverlayLayout, 0, 0, 4, 4);
  mMainLayout->addWidget(mControl, 0, 0, 4, 2, Qt::AlignLeft);

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
  //glEnable(GL_LINE_SMOOTH);
  //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  //glEnable(GL_MULTISAMPLE);
  
  // init game objects
  mEngine->initGL(this);
}

void GameWidget::paintGL()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);// | GL_STENCIL_BUFFER_BIT);
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
  mEngine->getWorld()->reset();
  emit mainMenu();
}
  
void GameWidget::updateInfo()
{
  // update player info
  double fps = mEngine->getFramerate();
  Point3f ppos = mEngine->getPlayer()->getPos();
  Point3i coll = mEngine->getPlayer()->getCollisions();
  Point3i cpos = World::chunkPos(ppos);
  CompleteBlock b = mEngine->getPlayer()->selectedBlock();
  float lightLevel = 0.0f;
  if(isFluidBlock(b.type))
    { lightLevel = reinterpret_cast<Fluid*>(b.data)->level; }
  
  QLabel *fpsl = mPlayerOverlay->getLabel(0);
  QLabel *pl = mPlayerOverlay->getLabel(1);
  QLabel *cl = mPlayerOverlay->getLabel(2);
  QLabel *tl = mPlayerOverlay->getLabel(3);
  QLabel *ll = mPlayerOverlay->getLabel(4);

  std::stringstream ss;  
  ss << fps << " FPS";
  fpsl->setText(ss.str().c_str());
  ss.str(""); ss.clear();
  
  ss << "Position: " << ppos;
  pl->setText(ss.str().c_str());
  ss.str(""); ss.clear();
  
  ss << "Chunk:    " << cpos;
  cl->setText(ss.str().c_str());
  ss.str(""); ss.clear();
  
  ss << "Level:    " << lightLevel;
  ll->setText(ss.str().c_str());
  tl->setText(("Type:     " + toString(b.type)).c_str());
  ss.str(""); ss.clear();
}

void GameWidget::setMaterial(block_t type)
{
  mEngine->setMaterial(type, nullptr);
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
      if(dPos.x() != 0 || dPos.y() != 0)
        {      
          InputData data;      
          data.type = input_t::MOUSE_MOVE;
          data.mouseMove.vPos = Vector2f{(float)event->pos().x() / width(),
                                         (float)event->pos().y() / height() };
          data.mouseMove.dPos = Vector2i{dPos.x(), dPos.y()};
          data.mouseMove.drag = mMouseDown;
          data.mouseMove.captured = mMouseCaptured || (event->modifiers() & Qt::ControlModifier);
          mEngine->sendInput(data);
        }
    }
  
  if(mMouseCaptured || (event->modifiers() & Qt::ControlModifier))
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
      data.action.keyDown = true;
      break;
    case Qt::Key_Control:
      data.type = input_t::ACTION_SNEAK;
      data.action.keyDown = true;
      break;
    case Qt::Key_Shift:
      data.type = input_t::ACTION_RUN;
      data.action.keyDown = true;
      break;

    case Qt::Key_P:
      mEngine->getWorld()->pauseFrustumCulling();
      break;
    case Qt::Key_G:
      mEngine->getPlayer()->toggleGodMode();
      break;

      #define ROTATE_SPEED 0.05
      
      // rotate chunk visualizer
    case Qt::Key_Left:
      mEngine->getWorld()->rotateVisualizer(Vector3f{-ROTATE_SPEED,0,0});
      break;
    case Qt::Key_Right:
      mEngine->getWorld()->rotateVisualizer(Vector3f{ROTATE_SPEED,0,0});
      break;
    case Qt::Key_Down:
      mEngine->getWorld()->rotateVisualizer(Vector3f{0,ROTATE_SPEED,0});
      break;
    case Qt::Key_Up:
      mEngine->getWorld()->rotateVisualizer(Vector3f{0,-ROTATE_SPEED,0});
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
      
    case Qt::Key_Space:
      data.type = input_t::ACTION_JUMP;
      data.action.keyDown = false;
      break;
    case Qt::Key_Control:
      data.type = input_t::ACTION_SNEAK;
      data.action.keyDown = false;
      break;
    case Qt::Key_Shift:
      data.type = input_t::ACTION_RUN;
      data.action.keyDown = false;
      break;

      // stop rotating chunk visualizer
    case Qt::Key_Left:
      mEngine->getWorld()->rotateVisualizer(Vector3f{ROTATE_SPEED,0,0});
      break;
    case Qt::Key_Right:
      mEngine->getWorld()->rotateVisualizer(Vector3f{-ROTATE_SPEED,0,0});
      break;
    case Qt::Key_Down:
      mEngine->getWorld()->rotateVisualizer(Vector3f{0,-ROTATE_SPEED,0});
      break;
    case Qt::Key_Up:
      mEngine->getWorld()->rotateVisualizer(Vector3f{0,ROTATE_SPEED,0});
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
