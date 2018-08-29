#include "gameWidget.hpp"

#include "logging.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>

#include <unistd.h>
#include <exception>
#include <functional>

#include <QString>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QCursor>
#include <QWidget>
#include <QTimer>

#include "block.hpp"
#include "player.hpp"
#include "vector.hpp"
#include "world.hpp"

#define RENDER_TIMESTEP_MS 5 //5 //ms
#define RENDER_TIMESTEP_S (RENDER_TIMESTEP_MS / 1000.0f)

//#define PLAYER_POS Point2i{0,0}

cGameWidget::cGameWidget(QWidget *parent, int numThreads, const std::string &worldName, uint32_t seed)
  : GlWidget(parent), mMousePos(-1000,-1000)
{
  mEngine = new VoxelEngine(this, numThreads, worldName, seed);
  
  mRenderTimer = new QTimer(this);
  connect(mRenderTimer, SIGNAL(timeout()), this, SLOT(update())); 
  mRenderTimer->start(RENDER_TIMESTEP_MS);
  
  mPosDisplay = new QTimer(this);
  connect(mPosDisplay, SIGNAL(timeout()), SLOT(sendPos()));
  mPosDisplay->start(10);

  setFocusPolicy((Qt::FocusPolicy)(Qt::StrongFocus));
  setFocus();
  grabKeyboard();
  setMouseTracking(true);
}

cGameWidget::~cGameWidget()
{
  LOGD("~cGameWidget()...");
  mRenderTimer->stop();
  mPosDisplay->stop();
  mEngine->stopPhysics();
  
  makeCurrent();
  LOGD("Cleaning up GL...");
  mEngine->cleanUpGL();
  LOGD("Done cleaning up GL.");
  doneCurrent();
  
  //delete mRenderTimer;
  //delete mPosDisplay;
  delete mEngine;
}

VoxelEngine* cGameWidget::getEngine()
{
  return mEngine;
}


void cGameWidget::glInit()
{
  GlWidget::glInit();
  glClearColor(0.229, 0.657, 0.921, 1.0);
  mEngine->initGL(this);
}

void cGameWidget::render()
{
  mEngine->render();
}


void cGameWidget::sendPos()
{
  Point3f ppos = mEngine->getPlayer()->getPos();
  Point3i coll = mEngine->getPlayer()->getCollisions();
  Point3i cpos = World::chunkPos(ppos);
  emit posChanged(ppos, coll, cpos);
  cBlock *b = mEngine->getPlayer()->selectedBlock();
  if(b)
    {
      emit blockInfo(b->type, b->lightLevel);
    }
  else
    {
      emit blockInfo(block_t::NONE, 0);
    }
}

void cGameWidget::setTool(block_t type)
{
  mEngine->setTool(type);
}

void cGameWidget::captureMouse(bool capture)
{
  mMouseCaptured = capture;
}
bool cGameWidget::getMouseCaptured() const
{ return mMouseCaptured; }

void cGameWidget::mousePressEvent(QMouseEvent *event)
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
void cGameWidget::mouseReleaseEvent(QMouseEvent *event)
{
  Q_UNUSED(event);
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
void cGameWidget::mouseMoveEvent(QMouseEvent *event)
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

void cGameWidget::keyPressEvent(QKeyEvent *event)
{
  InputData data;
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
      
    default:
      event->ignore();
      return;
    }
  
  mEngine->sendInput(data);
  event->accept();
}

void cGameWidget::keyReleaseEvent(QKeyEvent *event)
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

void cGameWidget::wheelEvent(QWheelEvent *event)
{
  /*
  static const int inertia = 80;
  static int position = 0;
  QPoint numPixels = event->pixelDelta();

  position += numPixels.y();
  while(position > inertia)
    {
      mEngine->nextTool();
      position -= inertia;
    }
  while(position < -inertia)
    {
      mEngine->prevTool();
      position += inertia;
    }
  
  event->accept();
  */
}

void cGameWidget::onResize(int w, int h)
{
  VoxelEngine::ProjDesc proj = mEngine->getProjection();
  proj.aspect = (float)w/(float)h;
  mEngine->setProjection(proj);
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
