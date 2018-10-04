#ifndef CHUNK_LOAD_WIDGET_HPP
#define CHUNK_LOAD_WIDGET_HPP

#include <QWidget>
#include <bitset>

class World;
class QPaintEvent;

class ChunkLoadWidget : public QWidget
{
  Q_OBJECT
  friend class ChunkDisplay;
public:
  typedef std::bitset<7> options_t;
  enum Option
    {
     OPT_CENTER = 0,
     OPT_RENDER,
     OPT_EMPTY,
     OPT_MESHED,
     OPT_READY,
     OPT_LOADED,
     OPT_LOADING,
     OPT_COUNT
    };
  
  ChunkLoadWidget(World *world, QWidget *parent = nullptr);
  virtual ~ChunkLoadWidget();
                            
                            

public slots:
  void setOptions(options_t opt);
  void setOption(Option opt, int state);
  void setZ(int z);
  
protected:
  void paintEvent(QPaintEvent *event);
  //void resizeEvent(QResizeEvent *event);
  //virtual void drawBackground(QPainter *painter, const QRectF &rect) override;
  //virtual void drawForeground(QPainter *painter, const QRectF &rect) override;
  
private:
  World *mWorld;
  options_t mOptions;
  int zLevel = 0;
};

#endif // CHUNK_LOAD_WIDGET_HPP
