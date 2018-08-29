#include "controlInterface.hpp"
#include "voxelEngine.hpp"

#include <iostream>

#include <QButtonGroup>
#include <QGroupBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QTabBar>
#include <QStackedLayout>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>


cControlInterface::cControlInterface(VoxelEngine *engine, QWidget *parent)
  : QWidget(parent), mEngine(engine)
{
  mMainLayout = new QGridLayout(this);

  mControlTabs = new QTabBar();
  mTabStackWidget = new QWidget();
  mTabStack = new QStackedLayout(mTabStackWidget);
  
  mMaterialWidget = new QWidget(this);
  mMaterialLayout = new QGridLayout(mMaterialWidget);
  mMaterialLayout->setMargin(0);
  mMaterialLayout->setSpacing(0);
  mMaterialLayout->addWidget(makeSimpleGroup(), 0, 0, 1, 1);
  
  mPhysicsWidget = new QWidget(this);
  mPhysicsLayout = new QGridLayout(mPhysicsWidget);
  mPhysicsLayout->setMargin(0);
  mPhysicsLayout->setSpacing(0);
  mPhysicsLayout->addWidget(makePhysicsGroup(), 0, 0, 1, 1);
  
  mLightWidget = new QWidget(this);
  mLightLayout = new QGridLayout(mLightWidget);
  mLightLayout->setMargin(0);
  mLightLayout->setSpacing(0);
  mLightLayout->addWidget(makeLightGroup(), 0, 0, 1, 1);
  
  mDebugWidget = new QWidget(this);
  mDebugLayout = new QGridLayout(mDebugWidget);
  mDebugLayout->setMargin(0);
  mDebugLayout->setSpacing(0);
  mDebugLayout->addWidget(makeDebugGroup(), 0, 0, 1, 1);

  mTabStack->addWidget(mMaterialWidget);
  mTabStack->addWidget(mPhysicsWidget);
  mTabStack->addWidget(mLightWidget);
  mTabStack->addWidget(mDebugWidget);
  
  mControlTabs->insertTab(0, QString("Materials"));
  mControlTabs->insertTab(1, QString("Physics"));
  mControlTabs->insertTab(2, QString("Lighting"));
  mControlTabs->insertTab(3, QString("Debug"));
  mControlTabs->setExpanding(false);

  mMainLayout->setSpacing(0);
  mMainLayout->setMargin(0);
  mMainLayout->addWidget(mControlTabs, 0, 0, 1, 1, Qt::AlignBottom);
  mMainLayout->addWidget(mTabStackWidget, 1, 0, 1, 1, Qt::AlignBottom);
  setLayout(mMainLayout);
  
  connect(mControlTabs, SIGNAL(currentChanged(int)), mTabStack, SLOT(setCurrentIndex(int)));
}

void cControlInterface::collapse()
{
  mTabStackWidget->hide();
}

void cControlInterface::expand()
{
  mTabStackWidget->show();
}

void cControlInterface::setTool(int id, bool checked)
{
  if(checked)
    { mEngine->setTool((block_t)id); }
}
void cControlInterface::setLightLevel(int level)
{
  LOGD("SETTING LIGHT LEVEL: %d", (int)level);
  mEngine->setLightLevel(level);
}
void cControlInterface::setWireframe(int wireframe)
{
  mEngine->setWireframe(wireframe != 0);
}
void cControlInterface::setDebug(int debug)
{
  mEngine->setDebug(debug != 0);
}

QGroupBox* cControlInterface::makeSimpleGroup()
{
  mSimpleGroup = new QButtonGroup(this);
  QGroupBox *group = new QGroupBox("Material");
  QVBoxLayout *vbox = new QVBoxLayout();

  for(int b = (int)simple_t::START; b < (int)simple_t::END; b++)
    {
      QRadioButton *r = new QRadioButton(toString((block_t)b).c_str());
      if(b == 0)
        { r->setChecked(true); }
      
      mSimpleGroup->addButton(r, b);
      vbox->addWidget(r);
    }

  vbox->setMargin(20);
  vbox->setSpacing(0);
  
  group->setLayout(vbox);
  connect(mSimpleGroup, SIGNAL(buttonToggled(int, bool)), this, SLOT(setTool(int, bool)));

  return group;
}


QGroupBox* cControlInterface::makeComplexGroup()
{
  return nullptr;
}
  
QGroupBox* cControlInterface::makeFluidGroup()
{
  return nullptr;
}



QGroupBox* cControlInterface::makePhysicsGroup()
{
  mPhysicsGroup = new QButtonGroup(this);
  QGroupBox *group = new QGroupBox("Physics");
  QVBoxLayout *vbox = new QVBoxLayout();
  QPushButton *b = new QPushButton("TEST");
  vbox->addWidget(b);
  vbox->setMargin(20);
  vbox->setSpacing(0);
  group->setLayout(vbox);
  return group;
}


QGroupBox* cControlInterface::makeLightGroup()
{
  mLightGroup = new QButtonGroup(this);
  QGroupBox *group = new QGroupBox("Light");
  QVBoxLayout *vbox = new QVBoxLayout();
  QHBoxLayout *sbox = new QHBoxLayout();
  
  QLabel *sLabel = new QLabel("Light Level");
  
  QSlider *lSlider = new QSlider(Qt::Horizontal);
  lSlider->setTickInterval(1);
  lSlider->setTickPosition(QSlider::TicksBelow);
  lSlider->setMinimum(1);
  lSlider->setMaximum(mEngine->mLightLevels);
  connect(lSlider, SIGNAL(sliderMoved(int)), this, SLOT(setLightLevel(int)));

  sbox->addWidget(sLabel);
  sbox->addWidget(lSlider);
  vbox->addLayout(sbox);
  vbox->setMargin(20);
  vbox->setSpacing(0);
  group->setLayout(vbox);
  return group;
}
QGroupBox* cControlInterface::makeDebugGroup()
{
  mDebugGroup = new QButtonGroup(this);
  QGroupBox *group = new QGroupBox("Debug");
  QVBoxLayout *vbox = new QVBoxLayout();
  
  QCheckBox *wireCb = new QCheckBox("Wire Frame");
  connect(wireCb, SIGNAL(stateChanged(int)), this, SLOT(setWireframe(int)));
  QCheckBox *debugCb = new QCheckBox("Debug");
  connect(debugCb, SIGNAL(stateChanged(int)), this, SLOT(setDebug(int)));
  
  vbox->addWidget(wireCb);
  vbox->addWidget(debugCb);
  vbox->setMargin(20);
  vbox->setSpacing(0);
  group->setLayout(vbox);
  return group;
}
