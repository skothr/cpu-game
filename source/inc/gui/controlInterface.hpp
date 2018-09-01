#ifndef CONTROL_INTERFACE_HPP
#define CONTROL_INTERFACE_HPP

#include <QWidget>
#include "block.hpp"
#include "world.hpp"

class QGridLayout;
class QVBoxLayout;
class QButtonGroup;
class QGroupBox;
class QTabBar;
class QStackedLayout;
class QSlider;

class VoxelEngine;

class cControlInterface : public QWidget
{
  Q_OBJECT
public:
  cControlInterface(VoxelEngine *engine, QWidget *parent = nullptr);

  void collapse();
  void expand();
  
signals:
    
protected slots:
  void setTool(int id, bool checked);
  void setLightLevel(int level);
  void setWireframe(int wireframe);
  void setDebug(int debug);
  void setFluidSim(int on);
  void setFluidEvap(int on);
  void setFluidLevel(int level);
  void setChunkRadiusXY(int radius);
  void setChunkRadiusZ(int radius);

protected:
  
  
private:
  VoxelEngine *mEngine = nullptr;
  World *mWorld = nullptr;
  QGridLayout *mMainLayout = nullptr;

  QTabBar *mControlTabs = nullptr;
  QWidget *mTabStackWidget = nullptr;
  QStackedLayout *mTabStack = nullptr;

  // active material selection tab
  QWidget *mMaterialWidget = nullptr;
  QGridLayout *mMaterialLayout = nullptr;
  QButtonGroup *mMaterialGroup = nullptr;
  QGroupBox* makeMaterialGroup();

  // physics control tab
  QWidget *mPhysicsWidget = nullptr;
  QGridLayout *mPhysicsLayout = nullptr;
  QButtonGroup *mPhysicsGroup = nullptr;
  QGroupBox* makePhysicsGroup();

  // lighting control tab
  QWidget *mLightWidget = nullptr;
  QGridLayout *mLightLayout = nullptr;
  QButtonGroup *mLightGroup = nullptr;
  QGroupBox* makeLightGroup();
  
  // debug control tab
  QWidget *mDebugWidget = nullptr;
  QGridLayout *mDebugLayout = nullptr;
  QButtonGroup *mDebugGroup = nullptr;
  QGroupBox* makeDebugGroup();

  block_t mTool = block_t::NONE;
  float mFluidLevel = 0.0f;
};

#endif // CONTROL_INTERFACE_HPP
