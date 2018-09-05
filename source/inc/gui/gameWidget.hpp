#ifndef GAME_WIDGET_HPP
#define GAME_WIDGET_HPP

#include <mutex>
#include <QVector2D>
#include <QOpenGLWidget>

#include "indexing.hpp"
#include "chunk.hpp"
#include "vector.hpp"
#include "voxelEngine.hpp"

class ControlInterface;
class PauseWidget;
class Overlay;


class QTimer;
class QStackedLayout;
class QGridLayout;

class GameWidget : public QOpenGLWidget
{
  Q_OBJECT
public:
  GameWidget(QWidget *parent = nullptr);
  virtual ~GameWidget();

  VoxelEngine* getEngine();
  void captureMouse(bool capture);
  bool getMouseCaptured() const;
  void setTool(block_t type);
  
  void start();
  void stop();
  void pause(bool status);

signals:
  void paused();
  void resumed();
  void quit();
             
public slots:
  void updateInfo();
  void resume();

protected:
  void initializeGL();
  void paintGL();
  void resizeGL(int w, int h);

  void render();

  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void keyReleaseEvent(QKeyEvent *event);
  void wheelEvent(QWheelEvent *event);

private:
  bool mMouseDown = false;
  bool mMouseCaptured = false;
  QPoint mMousePos;
  
  QTimer *mInfoTimer = nullptr;
  QTimer *mRenderTimer = nullptr;
  VoxelEngine *mEngine = nullptr;
  
  QStackedLayout *mOverlayLayout = nullptr;
  Overlay *mPlayerOverlay = nullptr;
  Overlay *mChunkOverlay = nullptr;
  PauseWidget *mPause = nullptr;
  ControlInterface *mControl = nullptr;
  QGridLayout *mMainLayout = nullptr;

};

#endif //GAME_WIDGET_HPP
