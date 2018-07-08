#ifndef GAME_WIDGET_HPP
#define GAME_WIDGET_HPP

#include <QVector2D>
#include <mutex>

#include "glWidget.hpp"
#include "vector.hpp"
#include "player.hpp"
#include "model.hpp"
#include "chunk.hpp"
#include "geometry.hpp"
#include "physics.hpp"

#include <QOpenGLTexture>

class QOpenGLBuffer;
class QTimer;
class cShader;
class cPlayer;

class cGameWidget : public GlWidget
{
  Q_OBJECT
public:
  cGameWidget(QWidget *parent = nullptr);
  virtual ~cGameWidget();

public slots:
  void stepPhysics(int us);
  void updateGraphics(int us);

  //public slots:
  //void updateGL();
  
protected:
  virtual void glInit() override;
  virtual void render() override;
  virtual void onResize(int w, int h) override;
  
  void wheelEvent(QWheelEvent *event);
  void resizeEvent(QResizeEvent *event);

  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void keyReleaseEvent(QKeyEvent *event);
  
private:
  //QTimer *mRender = nullptr;
  bool mMouseDown = false;
  cShader *mBlockShader = nullptr;
  cShader *mWireShader = nullptr;
  cChunk mChunk;
  cPlayer mPlayer;
  QPoint mMousePos;
  
  //QThread *mThread = nullptr;
  //bool mPhysicsRunning = false;
  //QTimer *mPhysics = nullptr;
  //double mPhysicsTimestep = 0.010;
  //double mPhysicsTime = 0.0;

  cTimedThread *mPhysics;
  cTimedThread *mRender;

  QOpenGLTexture mTex;
  
  float aspect = 1.0;
  float fov = 45.0;
  float zNear = 0.01;
  float zFar = 1000.0;
  Matrix4 mProjMat;
  
  GLuint mPosAttr;
  //cModel mCube;
};

#endif //GAME_WIDGET_HPP
