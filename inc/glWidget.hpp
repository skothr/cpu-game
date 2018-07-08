#ifndef GL_WIDGET_HPP
#define GL_WIDGET_HPP

#include <QtWidgets>
#include <QOpenGLWidget>
//#include <QOpenGLFunctions_4_3_Core>

class GlWidget : public QOpenGLWidget//, protected QOpenGLFunctions
{
  Q_OBJECT
public:
  explicit GlWidget(QWidget *parent = nullptr);
  virtual ~GlWidget();
  
protected:
  void initializeGL();
  void paintGL();
  void resizeGL(int w, int h);

  QSize minimumSizeHint() const;
  QSize sizeHint() const;

  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent *event);

  void wheelEvent(QWheelEvent *event);

  virtual void glInit();//QOpenGLFunctions *glf);
  virtual void render() = 0;//QOpenGLFunctions *glf) = 0;

  virtual void onResize(int w, int h)
  { Q_UNUSED(w); Q_UNUSED(h); }

private:
};

#endif //GL_WIDGET_HPP
