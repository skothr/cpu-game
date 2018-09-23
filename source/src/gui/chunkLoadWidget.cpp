#include "chunkLoadWidget.hpp"
#include "world.hpp"
#include <QGridLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>

ChunkLoadWidget::ChunkLoadWidget(World *world, QWidget *parent)
  : QWidget(parent), mWorld(world)
{
  //mLayout = new QGridLayout();
}
ChunkLoadWidget::~ChunkLoadWidget()
{

}
  
void ChunkLoadWidget::paintEvent(QPaintEvent *event)
{
  if(!mWorld)
    { LOGW("Chunk load widget has null world!!"); }
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  static const int rectW = 5;
  static const int rectH = 5;
  static const int rectPad = 1;
  static const int borderChunks = 1;


  Point3i center = mWorld->getCenter();
  Vector3i radius = mWorld->getRadius();
  Point3i min = center - radius;
  
  const int chunkW = (rectW+rectPad) * (radius[0]*2 + 1 + 2*borderChunks) - rectPad;
  const int chunkH = (rectH+rectPad) * (radius[1]*2 + 1 + 2*borderChunks) - rectPad;
  const Vector2i zSpacing{ chunkW, chunkH };

  int col = radius[2] - 1;
  Point3i p;
  for(int z = 0; z < radius[2]*2+1; z++)
    {
      int row = (z < radius[2] ? 2 : (z > radius[2] ? 0 : 1));
      
      for(int x = 0; x < radius[0]*2+1 + 2*borderChunks; x++)
        for(int y = 0; y < radius[1]*2+1 + 2*borderChunks; y++)
          {
            p[0] = min[0] - borderChunks + x;
            p[1] = min[1] - borderChunks + y;
            p[2] = min[2] + z;
            QPainterPath path;
            path.addRoundedRect(QRectF((rectW+rectPad) * x + zSpacing[0] * col,
                                       (rectH+rectPad) * y + zSpacing[1] * row,
                                       rectW, rectH), 1, 1 );
            if(mWorld->chunkIsEmpty(p))
              { painter.fillPath(path, Qt::gray); }
            else if(mWorld->chunkIsMeshed(p))
              { painter.fillPath(path, Qt::green); }
            else if(mWorld->chunkIsLoaded(p))
              { painter.fillPath(path, Qt::cyan); }
            else if(mWorld->chunkIsLoading(p))
              { painter.fillPath(path, Qt::yellow); }
            else
              { painter.fillPath(path, Qt::red); }
              
            painter.drawPath(path);
            if(p == center)
              {
                QPen pen(Qt::magenta, 3);
                QPen old = painter.pen(); 
                painter.setPen(pen);
                painter.drawRect(QRectF(zSpacing[0] * col, zSpacing[1] * row,
                                        (radius[0]*2 + 1 + 2*borderChunks) * (rectW + rectPad),
                                        (radius[1]*2 + 1 + 2*borderChunks) * (rectH + rectPad) ));
                painter.setPen(old);
              }

          }
      if(z < radius[2] - 1)
        { col--; }
      else if(z > radius[2])
        { col++; }
    }
}
