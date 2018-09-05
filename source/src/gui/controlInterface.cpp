#include "controlInterface.hpp"
#include "voxelEngine.hpp"
#include "button.hpp"
#include "fluid.hpp"

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


ControlInterface::ControlInterface(VoxelEngine *engine, QWidget *parent)
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

void ControlInterface::collapse()
{
  mTabStackWidget->hide();
}

void ControlInterface::expand()
{
  mTabStackWidget->show();
}

void ControlInterface::setTool(int id, bool checked)
{
  if(checked)
    {
      mTool = (block_t)id;
      if(isSimpleBlock(mTool))
        {
          mEngine->setTool(mTool, nullptr);
        }
      else if(isFluidBlock(mTool))
        {
          LOGD("SETTING TOOL: %s (%d)", toString(mTool).c_str(), (int)mFluidLevel);
          BlockData *data = new Fluid(mTool, gFluidEvap[mTool], mFluidLevel);
          mEngine->setTool(mTool, data);
          delete data;
        }
    }
}
void ControlInterface::setLightLevel(int level)
{
  LOGD("SETTING LIGHT LEVEL: %d", (int)level);
  mEngine->setLightLevel(level);
}
void ControlInterface::setWireframe(int wireframe)
{
  mEngine->setWireframe(wireframe != 0);
}
void ControlInterface::setDebug(int debug)
{
  mEngine->setDebug(debug != 0);
}
void ControlInterface::clearFluids()
{
  mWorld->clearFluids();
}
void ControlInterface::setFluidSim(int on)
{
  mWorld->setFluidSim(on != 0);
}
void ControlInterface::setFluidEvap(int on)
{
  mWorld->setFluidEvap(on != 0);
}
void ControlInterface::setChunkRadiusX(int radius)
{
  Vector3i rad = mWorld->getRadius();
  rad[0] = radius;
  mWorld->setRadius(rad);
}
void ControlInterface::setChunkRadiusY(int radius)
{
  Vector3i rad = mWorld->getRadius();
  rad[1] = radius;
  mWorld->setRadius(rad);
}
void ControlInterface::setChunkRadiusZ(int radius)
{
  Vector3i rad = mWorld->getRadius();
  rad[2] = radius;
  mWorld->setRadius(rad);
}
void ControlInterface::setFluidLevel(int level)
{
  std::cout << level << "\n";
  mFluidLevel = (float)level / 255.0f;
  std::cout << mFluidLevel << "\n";
  LOGD("SET FLUID LEVEL: %f", mFluidLevel);
  if(isFluidBlock(mTool))
    {
      LOGD("SETTING TOOL: %s (%f)", toString(mTool).c_str(), mFluidLevel);
      BlockData *data = new Fluid(mTool, gFluidEvap[mTool], mFluidLevel);
      mEngine->setTool(mTool, data);
      delete data;
    }
}

void ControlInterface::initRadius(const Vector3i &rad)
{
  mXSlider->setValue(rad[0]);
  mYSlider->setValue(rad[1]);
  mZSlider->setValue(rad[2]);
}

QGroupBox* ControlInterface::makeMaterialGroup()
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
  Button *clearBtn = new Button("Clear Fluids");
  connect(clearBtn, SIGNAL(clicked()), this, SLOT(clearFluids()));
  vbox->addWidget(clearBtn);
  
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
  levelS->setValue(255);
  connect(levelS, SIGNAL(sliderMoved(int)), this, SLOT(setFluidLevel(int)));

  hbox2->addWidget(levelL);
  hbox2->addWidget(levelS);
  vbox2->addLayout(hbox2);
  hbox->addLayout(vbox2);
  
  group->setLayout(hbox);
  connect(mMaterialGroup, SIGNAL(buttonToggled(int, bool)), this, SLOT(setTool(int, bool)));

  return group;
}

QGroupBox* ControlInterface::makePhysicsGroup()
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


QGroupBox* ControlInterface::makeLightGroup()
{
  mLightGroup = new QButtonGroup(this);
  QGroupBox *group = new QGroupBox("Light");
  QGridLayout *grid = new QGridLayout();
  
  // slider to adjust global daylight
  QLabel *sLabel = new QLabel("Light Level");
  QSlider *lSlider = new QSlider(Qt::Horizontal);
  lSlider->setTickInterval(1);
  lSlider->setTickPosition(QSlider::TicksBelow);
  lSlider->setMinimum(1);
  lSlider->setMaximum(mEngine->mLightLevels);
  lSlider->setValue(4);
  connect(lSlider, SIGNAL(sliderMoved(int)), this, SLOT(setLightLevel(int)));
  grid->addWidget(sLabel, 0,0,1,1);
  grid->addWidget(lSlider,0,1,1,3);
  
  // Sliders to adjust chunk load radius
  QLabel *xLabel = new QLabel("X Load Radius");
  mXSlider = new QSlider(Qt::Horizontal);
  mXSlider->setTickInterval(1);
  mXSlider->setTickPosition(QSlider::TicksBelow);
  mXSlider->setMinimum(0);
  mXSlider->setMaximum(16);
  mXSlider->setValue(mWorld->getRadius()[0]);
  connect(mXSlider, SIGNAL(sliderMoved(int)), this, SLOT(setChunkRadiusX(int)));
  grid->addWidget(xLabel, 0,7,1,1);
  grid->addWidget(mXSlider,0,8,1,3);
  
  QLabel *yLabel = new QLabel("Y Load Radius");
  mYSlider = new QSlider(Qt::Horizontal);
  mYSlider->setTickInterval(1);
  mYSlider->setTickPosition(QSlider::TicksBelow);
  mYSlider->setMinimum(0);
  mYSlider->setMaximum(16);
  mYSlider->setValue(mWorld->getRadius()[0]);
  connect(mYSlider, SIGNAL(sliderMoved(int)), this, SLOT(setChunkRadiusY(int)));
  grid->addWidget(yLabel, 1,7,1,1);
  grid->addWidget(mYSlider,1,8,1,3);
  
  QLabel *zLabel = new QLabel("Z Load Radius");
  mZSlider = new QSlider(Qt::Horizontal);
  mZSlider->setTickInterval(1);
  mZSlider->setTickPosition(QSlider::TicksBelow);
  mZSlider->setMinimum(0);
  mZSlider->setMaximum(8);
  mZSlider->setValue(mWorld->getRadius()[2]);
  connect(mZSlider, SIGNAL(sliderMoved(int)), this, SLOT(setChunkRadiusZ(int)));
  grid->addWidget(zLabel, 2,7,1,1);
  grid->addWidget(mZSlider,2,8,1,3);
  
  group->setLayout(grid);
  return group;
}
QGroupBox* ControlInterface::makeDebugGroup()
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
