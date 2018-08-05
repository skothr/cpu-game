#include "glWidget.hpp"

#include <iostream>
//#include <QOpenGLFunctions_4_3_Core>
#include <QSurface>

GlWidget::GlWidget(QWidget *parent)
  : QOpenGLWidget(parent)
{
  
  QSurfaceFormat format;
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  format.setVersion(4, 3);
  //format.setProfile(QSurfaceFormat::CoreProfile);
  format.setSamples(1);
  format.setRenderableType(QSurfaceFormat::OpenGL);
  setFormat(format);
  //create();
  
  /*  
  mContext = getContext(); //new QOpenGLContext();
  mContext->setFormat(format);
  mContext->create();

  mContext->makeCurrent((QSurface*)QWidget::parent());
  */
  //mContext = new QOpenGLContext();
  //mContext->create();

  //mContext->makeCurrent((QSurface*)QWidget::parent());
  

  /*
  mFuncs = mContext->versionFunctions<QOpenGLFunctions_4_3_Core>();
  if(!mFuncs)
    {
      std::cout << "Failed to obtain OpenGL versions object.\n";
      exit(1);
    }
  mFuncs->initializeOpenGLFunctions();
implem
  if(mContext->hasExtension(QByteArrayLiteral("GL_ARB_gpu_shader_fp64")))
    {
      std::cout << "Has Double precision!! :)\n";
    }
  else
    {
      std::cout << "No Double precision!! :(\n";
    }
  */
  
}

GlWidget::~GlWidget()
{
  std::cout << "~GlWidget()...\n";
  //makeCurrent();
  //doneCurrent();
}

void GlWidget::glInit()
{
  
}

void GlWidget::initializeGL()
{
  //initializeOpenGLFunctions();
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //glDepthFunc(GL_LESS);
  glInit();
}

void GlWidget::paintGL()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  render();
}


void GlWidget::resizeGL(int w, int h)
{
  onResize(w, h);
  glViewport(0, 0, w, h);
}

QSize GlWidget::minimumSizeHint() const
{
  return QSize(128, 128);
}

QSize GlWidget::sizeHint() const
{
  return QSize(1080, 1080);
}

void GlWidget::wheelEvent(QWheelEvent *event)
{
  Q_UNUSED(event);
  update();
}

void GlWidget::mousePressEvent(QMouseEvent *event)
{
  Q_UNUSED(event);
  update();
}

void GlWidget::mouseMoveEvent(QMouseEvent *event)
{
  Q_UNUSED(event);
  //update();
}


void GlWidget::keyPressEvent(QKeyEvent *event)
{
  switch(event->key())
    {
    case Qt::Key_Escape:
      std::cout << "Escape pressed (GlWidget).\n";
      close();
      break;

    default:
      break;
    }
}
