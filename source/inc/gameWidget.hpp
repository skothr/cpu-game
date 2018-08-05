#ifndef GAME_WIDGET_HPP
#define GAME_WIDGET_HPP

#include <mutex>
#include <QVector2D>

#include "glWidget.hpp"
#include "vector.hpp"
#include "geometry.hpp"
#include "textureAtlas.hpp"
#include "voxelEngine.hpp"

class QTimer;

class cGameWidget : public GlWidget
{
  Q_OBJECT
public:
  cGameWidget(QWidget *parent = nullptr, int numThreads = 1,
	      const std::string &worldName = "", uint32_t seed = 0 );
  virtual ~cGameWidget();

signals:
  void posChanged(Point3f player, Point3i collisions, Point3i chunk);

public slots:
  void sendPos();
  
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

  void renderUpdate();
  
private:
  bool mMouseDown = false;
  QPoint mMousePos;
  
  QTimer *mPosDisplay = nullptr;
  QTimer *mRenderTimer = nullptr;
  cVoxelEngine *mEngine = nullptr;
};

#endif //GAME_WIDGET_HPP