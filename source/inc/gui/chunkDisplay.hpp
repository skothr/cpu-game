#ifndef CHUNK_DISPLAY
#define CHUNK_DISPLAY

#include <QWidget>

class ChunkLoadWidget;
class World;
class DisplaySlider;

class ChunkDisplay : public QWidget
{
  Q_OBJECT
public:
  ChunkDisplay(World *world, QWidget *parent = nullptr);

private: 
  ChunkLoadWidget *mChunkWidget = nullptr;
};

#endif // CHUNK_DISPLAY
