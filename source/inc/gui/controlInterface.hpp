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

class DisplaySlider;

class VoxelEngine;

class ControlInterface : public QWidget
{
  Q_OBJECT
public:
  ControlInterface(VoxelEngine *engine, QWidget *parent = nullptr);

  void collapse();
  void expand();
  
  void initRadius(const Vector3i &rad);
  
signals:
    
protected slots:
  void setTool(int id, bool checked);
  void setLightLevel(int level);
  void setWireframe(int wireframe);
  void setDebug(int debug);
  void clearFluids();
  void setFluidSim(int on);
  void setFluidEvap(double evap);
  void setFluidLevel(int level);
  void setRaytrace(int on);
  void setChunkRadiusX(int radius);
  void setChunkRadiusY(int radius);
  void setChunkRadiusZ(int radius);

protected:
  
  
private:
  VoxelEngine *mEngine = nullptr;
  World *mWorld = nullptr;
  QGridLayout *mMainLayout = nullptr;

  QTabBar *mControlTabs = nullptr;
  QWidget *mTabStackWidget = nullptr;
  QStackedLayout *mTabStack = nullptr;

  DisplaySlider *mXSlider = nullptr;
  DisplaySlider *mYSlider = nullptr;
  DisplaySlider *mZSlider = nullptr;

  QButtonGroup *mMaterialGroup = nullptr;
  
  // active material selection tab
  QWidget *mSimpleWidget = nullptr;
  QGridLayout *mSimpleLayout = nullptr;
  QGroupBox* makeSimpleGroup();
  
  QWidget *mComplexWidget = nullptr;
  QGridLayout *mComplexLayout = nullptr;
  QGroupBox* makeComplexGroup();
  
  QWidget *mFluidWidget = nullptr;
  QGridLayout *mFluidLayout = nullptr;
  QGroupBox* makeFluidGroup();

  // physics control tab
  QWidget *mPhysicsWidget = nullptr;
  QGridLayout *mPhysicsLayout = nullptr;
  QButtonGroup *mPhysicsGroup = nullptr;
  QGroupBox* makePhysicsGroup();

  // lighting control tab
  QWidget *mRenderWidget = nullptr;
  QGridLayout *mRenderLayout = nullptr;
  QButtonGroup *mRenderGroup = nullptr;
  QGroupBox* makeRenderGroup();
  
  // debug control tab
  QWidget *mDebugWidget = nullptr;
  QGridLayout *mDebugLayout = nullptr;
  QButtonGroup *mDebugGroup = nullptr;
  QGroupBox* makeDebugGroup();

  block_t mTool = block_t::NONE;
  float mFluidLevel = 1.0f;
};

#endif // CONTROL_INTERFACE_HPP
