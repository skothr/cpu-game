#ifndef WORLD_LOAD_HPP
#define WORLD_LOAD_HPP

#include <QFrame>
#include <vector>
#include "world.hpp"

class QLabel;
class QListWidget;

class WorldLoad : public QWidget
{
  Q_OBJECT
public:
  WorldLoad(World *world, QWidget *parent = nullptr);
  virtual ~WorldLoad() { }
  
signals:
  void loaded(const World::Options &opt);
  void back();
protected slots:
  void setChunkRadiusX(int rx);
  void setChunkRadiusY(int ry);
  void setChunkRadiusZ(int rz);
  void setLoadThreads(int threads);
  void setMeshThreads(int threads);
  void selectWorld(int index);
  void loadWorld();
private:
  World::Options mOptions;
  std::vector<World::Options> mWorlds;
  World *mWorld = nullptr;
  QLabel *mNameLabel = nullptr;
  QLabel *mTerrainLabel = nullptr;
  QLabel *mSeedLabel = nullptr;
  QListWidget *mWorldList = nullptr;
  
  void loadWorlds();
};

#endif // WORLD_LOAD_HPP
