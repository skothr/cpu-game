#include "chunkLoadWidget.hpp"
#include "world.hpp"
#include <QGridLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>

ChunkLoadWidget::ChunkLoadWidget(World *world, QWidget *parent)
  : QWidget(parent), mWorld(world), mOptions(std::string("1111111"))
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setMinimumHeight(256);
  setMinimumWidth(256);
  //setSizeHint(QSize(256,256));
}
ChunkLoadWidget::~ChunkLoadWidget()
{

}

void ChunkLoadWidget::setOptions(options_t opt)
{ mOptions = opt; }

void ChunkLoadWidget::setOption(Option opt, int state)
{
  mOptions[opt] = state;
}

void ChunkLoadWidget::setZ(int z)
{ zLevel = z; }


// void ChunkLoadWidget::drawBackground(QPainter *painter, const QRectF &rect)
// {

// }
// void ChunkLoadWidget::drawForeground(QPainter *painter, const QRectF &rect)
// {

// }

void ChunkLoadWidget::paintEvent(QPaintEvent *event)
{
  if(!mWorld)
    { LOGW("Chunk load widget has null world!!"); }
  QPainter painter(this);
  //painter.setRenderHint(QPainter::Antialiasing);

  static const float padding = 2;
  static const int borderChunks = 2;

  const Point3i center = mWorld->getCenter();
  const Vector3i radius = mWorld->getRadius();
  const Point3i min = center - radius;
  
  const Vector3i chunkDim = radius*2 + 1 + 2*borderChunks;
  
  const float rectW = (width() - padding) / (chunkDim[0]);
  const float rectH = (height() - padding) / (chunkDim[1]);

  const float rectSize = std::min(rectW, rectH) - padding;
  const Vector2f margin = Vector2f{width() - padding - (rectSize+padding)*(chunkDim[0]),
                                   height() - padding - (rectSize+padding)*(chunkDim[1]) } / 2.0f + padding;
  
  Point3i p;
  for(int x = 0; x < chunkDim[0]; x++)
    for(int y = 0; y < chunkDim[1]; y++)
      {
        p[0] = min[0] - borderChunks + x;
        p[1] = min[1] - borderChunks + (chunkDim[1]-y-1);
        p[2] = center[2] + zLevel;
        QPainterPath path;
        path.addRoundedRect(QRectF(margin[0] + (rectSize+padding) * x,
                                   margin[1] + (rectSize+padding) * y, rectSize, rectSize), 1, 1 );
        if(mOptions[OPT_EMPTY] && mWorld->chunkIsEmpty(p))
          { painter.fillPath(path, Qt::darkGray); }
        else if(mOptions[OPT_MESHED] && mWorld->chunkIsMeshed(p))
          { painter.fillPath(path, Qt::green); }
        else if(mOptions[OPT_MESHED] && mWorld->chunkIsMeshing(p))
          { painter.fillPath(path, Qt::magenta); }
        else if(mOptions[OPT_READY] && mWorld->chunkIsReady(p))
          { painter.fillPath(path, Qt::darkGreen); }
        else if(mOptions[OPT_LOADED] && mWorld->chunkIsLoaded(p))
          { painter.fillPath(path, Qt::darkCyan); }
        else if(mOptions[OPT_LOADING] && mWorld->chunkIsLoading(p))
          { painter.fillPath(path, Qt::darkYellow); }
        else
          { painter.fillPath(path, Qt::darkRed); }
              
        painter.drawPath(path);

        // draw outline round square
        QColor col;
        bool draw = false;
        if(mOptions[OPT_CENTER] && p == center)
          {
            col = Qt::blue;
            draw = true;
          }
        else if(mOptions[OPT_RENDER] && mWorld->chunkIsVisible(p))
          {
            col = Qt::white;
            draw = true;
          }
        
        if(draw)
          {
            QPen old = painter.pen();
            QPen pen(col);
            pen.setWidth(3);
            painter.setPen(pen);
            painter.drawRect(QRectF(margin[0] + (rectSize+padding) * x,
                                    margin[1] + (rectSize+padding) * y, rectSize, rectSize));
            painter.setPen(old);
          }
      }
}
