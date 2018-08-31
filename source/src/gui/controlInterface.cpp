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
  : QWidget(parent), mEngine(engine), mWorld(engine->getWorld())
{
  mMainLayout = new QGridLayout(this);

  mControlTabs = new QTabBar();
  mTabStackWidget = new QWidget();
  mTabStack = new QStackedLayout(mTabStackWidget);
  
  mMaterialWidget = new QWidget(this);
  mMaterialLayout = new QGridLayout(mMaterialWidget);
  mMaterialLayout->setMargin(0);
  mMaterialLayout->setSpacing(0);
  mMaterialLayout->addWidget(makeMaterialGroup(), 0, 0, 1, 1);
  
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
    {
      mTool = (block_t)id;
      if(id < (int)simple_t::END)
        {
          mEngine->setTool(mTool, nullptr);
        }
      else if(id < (int)fluid_t::END)
        {
          LOGD("SETTING TOOL: %s (%d)", toString(mTool).c_str(), (int)mFluidLevel);
          BlockData *data = new FluidData(gFluidEvap[fluidIndex((block_t)id)], mFluidLevel);
          mEngine->setTool(mTool, data);
          delete data;
        }
    }
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
void cControlInterface::setFluidSim(int on)
{
  mWorld->setFluidSim(on != 0);
}
void cControlInterface::setFluidEvap(int on)
{
  mWorld->setFluidEvap(on != 0);
}

void cControlInterface::setFluidLevel(int level)
{
  std::cout << level << "\n";
  mFluidLevel = (float)level / 255.0f;
  std::cout << mFluidLevel << "\n";
  LOGD("SET FLUID LEVEL: %f", mFluidLevel);
  if(isFluidBlock(mTool))
    {
      LOGD("SETTING TOOL: %s (%f)", toString(mTool).c_str(), mFluidLevel);
      BlockData *data = new FluidData(gFluidEvap[fluidIndex((block_t)mTool)], mFluidLevel);
      mEngine->setTool(mTool, data);
      delete data;
    }
}

QGroupBox* cControlInterface::makeMaterialGroup()
{
  mMaterialGroup = new QButtonGroup(this);
  QGroupBox *group = new QGroupBox("Materials");
  QVBoxLayout *vbox = new QVBoxLayout();
  QHBoxLayout *hbox = new QHBoxLayout();

              
  // simple group
  QVBoxLayout *svbox = new QVBoxLayout();

  for(int b = (int)simple_t::START; b < (int)simple_t::END; b++)
    {
      QRadioButton *r = new QRadioButton(toString((block_t)b).c_str());
      if(b == 0)
        { r->setChecked(true); }
      
      mMaterialGroup->addButton(r, b);
      svbox->addWidget(r);
    }
  svbox->setMargin(20);
  svbox->setSpacing(0);

  hbox->addLayout(svbox);

  // fluid group
  for(int b = (int)fluid_t::START; b < (int)fluid_t::END; b++)
    {
      QRadioButton *r = new QRadioButton(toString((block_t)b).c_str());
      if(b == 0)
        { r->setChecked(true); }
      
      mMaterialGroup->addButton(r, b);
      vbox->addWidget(r);
    }
  vbox->setMargin(20);
  vbox->setSpacing(0);

  hbox->addLayout(vbox);

  QVBoxLayout *vbox2 = new QVBoxLayout();
  QHBoxLayout *hbox2 = new QHBoxLayout();
  QLabel *levelL = new QLabel("Fluid Level");
  QSlider *levelS = new QSlider(Qt::Horizontal);
  levelS->setTickInterval(1);
  levelS->setTickPosition(QSlider::TicksBelow);
  levelS->setMinimum(0);
  levelS->setMaximum(255);
  connect(levelS, SIGNAL(sliderMoved(int)), this, SLOT(setFluidLevel(int)));

  hbox2->addWidget(levelL);
  hbox2->addWidget(levelS);
  vbox2->addLayout(hbox2);
  hbox->addLayout(vbox2);
  
  group->setLayout(hbox);
  connect(mMaterialGroup, SIGNAL(buttonToggled(int, bool)), this, SLOT(setTool(int, bool)));

  return group;
}

QGroupBox* cControlInterface::makePhysicsGroup()
{
  mPhysicsGroup = new QButtonGroup(this);
  QGroupBox *group = new QGroupBox("Physics");
  QGridLayout *grid = new QGridLayout();
  grid->setMargin(20);
  grid->setSpacing(0);


  QCheckBox *fluidSimCb = new QCheckBox("Fluid Physics");
  fluidSimCb->setChecked(true);
  connect(fluidSimCb, SIGNAL(stateChanged(int)), this, SLOT(setFluidSim(int)));
  grid->addWidget(fluidSimCb, 0,0,1,1);
  QCheckBox *fluidEvapCb = new QCheckBox("Fluid Evaporation");
  fluidEvapCb->setChecked(true);
  connect(fluidEvapCb, SIGNAL(stateChanged(int)), this, SLOT(setFluidEvap(int)));
  grid->addWidget(fluidEvapCb, 1,0,1,1);
  
  group->setLayout(grid);
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
