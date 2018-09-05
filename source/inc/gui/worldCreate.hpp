#ifndef WORLD_CREATE_HPP
#define WORLD_CREATE_HPP

#include <QWidget>
#include <vector>
#include "world.hpp"

class QLabel;

class WorldCreate : public QWidget
{
  Q_OBJECT
public:
  WorldCreate(QWidget *parent = nullptr);
  virtual ~WorldCreate() { }

signals:
  void created(const World::Options &opt);
  void back();
protected slots:
  void setName(const QString &name);
  void setTerrain(int id);
  void setSeed(const QString &name);
  void setChunkRadiusX(int rx);
  void setChunkRadiusY(int ry);
  void setChunkRadiusZ(int rz);
  void setLoadThreads(int threads);
  void setMeshThreads(int threads);
  void createWorld();
  
protected:
private:
  World::Options mOptions;
};

#endif // WORLD_CREATE_HPP
