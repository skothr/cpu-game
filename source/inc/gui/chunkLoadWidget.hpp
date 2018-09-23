#ifndef CHUNK_LOAD_WIDGET_HPP
#define CHUNK_LOAD_WIDGET_HPP

#include <QWidget>

class World;
class QGridLayout;
class QPaintEvent;

class ChunkLoadWidget : public QWidget
{
public:
  ChunkLoadWidget(World *world, QWidget *parent = nullptr);
  virtual ~ChunkLoadWidget();
  
protected:
  void paintEvent(QPaintEvent *event);
  
private:
  World *mWorld;
  QGridLayout *mLayout = nullptr;
};

#endif // CHUNK_LOAD_WIDGET_HPP
