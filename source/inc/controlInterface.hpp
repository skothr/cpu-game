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

class cVoxelEngine;

class cControlInterface : public QWidget
{
  Q_OBJECT
  
public:
  cControlInterface(cVoxelEngine *engine, QWidget *parent = nullptr);

  void collapse();
  void expand();
  
signals:
    
protected slots:
  void setTool(int id, bool checked);
  void setLightLevel(int level);

protected:
  
  
private:
  cVoxelEngine *mEngine = nullptr;
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
};

#endif // CONTROL_INTERFACE_HPP
