#ifndef CONTROL_INTERFACE_HPP
#define CONTROL_INTERFACE_HPP

#include <QWidget>

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

protected:
  
  
private:
  VoxelEngine *mEngine = nullptr;
  QGridLayout *mMainLayout = nullptr;

  QTabBar *mControlTabs = nullptr;
  QWidget *mTabStackWidget = nullptr;
  QStackedLayout *mTabStack = nullptr;

  // active material selection tab
  QWidget *mMaterialWidget = nullptr;
  QGridLayout *mMaterialLayout = nullptr;
  QButtonGroup *mSimpleGroup = nullptr;
  QButtonGroup *mComplexGroup = nullptr;
  QButtonGroup *mFluidGroup = nullptr;
  QGroupBox* makeSimpleGroup();
  QGroupBox* makeComplexGroup();
  QGroupBox* makeFluidGroup();

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
};

#endif // CONTROL_INTERFACE_HPP
