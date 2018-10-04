#include "controlInterface.hpp"
#include "voxelEngine.hpp"
#include "button.hpp"
#include "fluid.hpp"

#include "displaySlider.hpp"
#include "chunkDisplay.hpp"

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
#include <QDoubleSpinBox>


ControlInterface::ControlInterface(VoxelEngine *engine, QWidget *parent)
  : QWidget(parent), mEngine(engine), mWorld(engine->getWorld())
{
  mMainLayout = new QGridLayout(this);
  mMaterialGroup = new QButtonGroup(this);
  mToolGroup = new QButtonGroup(this);
  connect(mMaterialGroup, SIGNAL(buttonToggled(int, bool)), this, SLOT(setMaterial(int, bool)));
  connect(mToolGroup, SIGNAL(buttonToggled(int, bool)), this, SLOT(setTool(int, bool)));

  mControlTabs = new QTabBar();
  mControlTabs->setShape(QTabBar::RoundedWest);
  mTabStackWidget = new QWidget();
  mTabStack = new QStackedLayout(mTabStackWidget);
  
  mSimpleWidget = new QWidget(this);
  mSimpleLayout = new QGridLayout(mSimpleWidget);
  mSimpleLayout->setMargin(0);
  mSimpleLayout->setSpacing(0);
  mSimpleLayout->addWidget(makeSimpleGroup(), 0, 0, 1, 1);
  
  mComplexWidget = new QWidget(this);
  mComplexLayout = new QGridLayout(mComplexWidget);
  mComplexLayout->setMargin(0);
  mComplexLayout->setSpacing(0);
  mComplexLayout->addWidget(makeComplexGroup(), 0, 0, 1, 1);
  
  mFluidWidget = new QWidget(this);
  mFluidLayout = new QGridLayout(mFluidWidget);
  mFluidLayout->setMargin(0);
  mFluidLayout->setSpacing(0);
  mFluidLayout->addWidget(makeFluidGroup(), 0, 0, 1, 1);
  
  mPhysicsWidget = new QWidget(this);
  mPhysicsLayout = new QGridLayout(mPhysicsWidget);
  mPhysicsLayout->setMargin(0);
  mPhysicsLayout->setSpacing(0);
  mPhysicsLayout->addWidget(makePhysicsGroup(), 0, 0, 1, 1);
  
  mRenderWidget = new QWidget(this);
  mRenderLayout = new QGridLayout(mRenderWidget);
  mRenderLayout->setMargin(0);
  mRenderLayout->setSpacing(0);
  mRenderLayout->addWidget(makeRenderGroup(), 0, 0, 1, 1);
  
  mDebugWidget = new QWidget(this);
  mDebugLayout = new QGridLayout(mDebugWidget);
  mDebugLayout->setMargin(0);
  mDebugLayout->setSpacing(0);
  mDebugLayout->addWidget(makeDebugGroup(), 0, 0, 1, 1);

  mTabStack->addWidget(mSimpleWidget);
  mTabStack->addWidget(mComplexWidget);
  mTabStack->addWidget(mFluidWidget);
  mTabStack->addWidget(mPhysicsWidget);
  mTabStack->addWidget(mRenderWidget);
  mTabStack->addWidget(mDebugWidget);
  
  mControlTabs->insertTab(0, QString("Blocks"));
  mControlTabs->insertTab(1, QString("Computing"));
  mControlTabs->insertTab(2, QString("Fluids"));
  mControlTabs->insertTab(3, QString("Physics"));
  mControlTabs->insertTab(4, QString("Rendering"));
  mControlTabs->insertTab(5, QString("Debug"));
  mControlTabs->setExpanding(false);

  connect(mControlTabs, SIGNAL(tabBarClicked(int)), this, SLOT(tabClicked(int)));

  mMainLayout->setSpacing(0);
  mMainLayout->setMargin(0);
  mMainLayout->addWidget(mControlTabs, 0, 1, 1, 1, Qt::AlignLeft|Qt::AlignTop);
  mMainLayout->addWidget(mTabStackWidget, 0, 0, 1, 1, Qt::AlignLeft);
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

void ControlInterface::tabClicked(int index)
{
  if(mTabStackWidget->isHidden())
    { expand(); }
  else if(index == mControlTabs->currentIndex())
    { collapse(); }
}

void ControlInterface::setMaterial(int id, bool checked)
{
  if(checked)
    {
      mMaterial = (block_t)id;
      if(isSimpleBlock(mMaterial))
        {
          mEngine->setMaterial(mMaterial, nullptr);
        }
      else if(isComplexBlock(mMaterial))
        {
          BlockData *data = nullptr;
          switch(mMaterial)
            {
            case block_t::DEVICE:
              data = new DeviceBlock();
              break;
            case block_t::MEMORY:
              data = new MemoryBlock(64, 100);
              break;
            case block_t::CPU:
              data = new CpuBlock(8, 8, 100);
              break;
            }
          mEngine->setMaterial(mMaterial, data);
          delete data;
        }
      else if(isFluidBlock(mMaterial))
        {
          LOGD("SETTING MATIERIAL: %s (%d)", toString(mMaterial).c_str(), (int)mFluidLevel);
          BlockData *data = new Fluid(mMaterial, mFluidLevel);
          mEngine->setMaterial(mMaterial, data);
          delete data;
        }
    }
}

void ControlInterface::setTool(int id, bool checked)
{
  mEngine->setTool((tool_t)(id + 1));
}
void ControlInterface::setToolRad(int rad)
{
  mEngine->setToolRad(rad);
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
void ControlInterface::setFrustumCulling(int on)
{
  mEngine->getWorld()->setFrustumCulling(on != 0);
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
void ControlInterface::setFluidEvap(double evap)
{
  mWorld->setFluidEvap(evap);
}
void ControlInterface::setRaytrace(int on)
{
  mWorld->setRaytracing(on != 0);
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
  if(isFluidBlock(mMaterial))
    {
      LOGD("SETTING MATERIAL: %s (%f)", toString(mMaterial).c_str(), mFluidLevel);
      BlockData *data = new Fluid(mMaterial, mFluidLevel);
      mEngine->setMaterial(mMaterial, data);
      delete data;
    }
}

void ControlInterface::initRadius(const Vector3i &rad)
{
  mXSlider->setValue(rad[0]);
  mYSlider->setValue(rad[1]);
  mZSlider->setValue(rad[2]);
}

QGroupBox* ControlInterface::makeSimpleGroup()
{
  QGroupBox *group = new QGroupBox("Blocks");
              
  // simple group
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setMargin(20);
  layout->setSpacing(0);

  for(int b = (int)simple_t::START; b < (int)simple_t::END; b++)
    {
      QRadioButton *r = new QRadioButton(toString((block_t)b).c_str());
      if(b == 0)
        { r->setChecked(true); }
      
      mMaterialGroup->addButton(r, b);
      layout->addWidget(r);
    }
  layout->addStretch(1);

  layout->addWidget(new QLabel("Tool:"));

  QRadioButton *sr = new QRadioButton("Sphere");
  sr->setChecked(true);
  mToolGroup->addButton(sr, 0);
  layout->addWidget(sr);
  QRadioButton *cr = new QRadioButton("Cube");
  mToolGroup->addButton(cr, 1);
  layout->addWidget(cr);
  QRadioButton *lr = new QRadioButton("Line");
  mToolGroup->addButton(lr, 2);
  layout->addWidget(lr);

  QSpinBox *radBox = new QSpinBox();
  radBox->setRange(1, 32);
  radBox->setValue(1);
  connect(radBox, SIGNAL(valueChanged(int)), this, SLOT(setToolRad(int)));

  QGridLayout *grid = new QGridLayout();
  grid->addWidget(new QLabel("Radius:"), 0,0,1,1);
  grid->addWidget(radBox, 0,1,1,1);
  
  layout->addLayout(grid);
  layout->addStretch(2);
  
  group->setLayout(layout);
  return group;
}

QGroupBox* ControlInterface::makeComplexGroup()
{
  QGroupBox *group = new QGroupBox("Computing");
  
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setMargin(20);
  layout->setSpacing(0);

  for(int b = (int)complex_t::START; b < (int)complex_t::END; b++)
    {
      QRadioButton *r = new QRadioButton(toString((block_t)b).c_str());
      mMaterialGroup->addButton(r, b);
      layout->addWidget(r);
    }
  layout->addStretch(1);
  
  group->setLayout(layout);
  return group;
}

QGroupBox* ControlInterface::makeFluidGroup()
{
  QGroupBox *group = new QGroupBox("Fluids");
  
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setMargin(20);
  layout->setSpacing(0);
  for(int b = (int)fluid_t::START; b < (int)fluid_t::END; b++)
    {
      QRadioButton *r = new QRadioButton(toString((block_t)b).c_str());
      mMaterialGroup->addButton(r, b);
      layout->addWidget(r);
    }
  Button *clearBtn = new Button("Clear Fluids");
  connect(clearBtn, SIGNAL(clicked()), this, SLOT(clearFluids()));
  layout->addWidget(clearBtn);
  
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
  layout->addLayout(hbox2);

  QLabel *eLabel = new QLabel("");
  QDoubleSpinBox *evap = new QDoubleSpinBox();
  evap->setRange(0.0, 1.0);
  evap->setSingleStep(0.00002);
  evap->setValue(0.0001);
  evap->setDecimals(5);
  connect(evap, SIGNAL(valueChanged(double)), this, SLOT(setFluidEvap(double)));

  QHBoxLayout *ebox = new QHBoxLayout();
  ebox->addWidget(eLabel);
  ebox->addWidget(evap);

  layout->addLayout(ebox);
  
  QCheckBox *fluidSimCb = new QCheckBox("Fluid Physics");
  fluidSimCb->setChecked(true);
  connect(fluidSimCb, SIGNAL(stateChanged(int)), this, SLOT(setFluidSim(int)));
  layout->addWidget(fluidSimCb);
  
  layout->addStretch(1);
  group->setLayout(layout);
  return group;
}

QGroupBox* ControlInterface::makePhysicsGroup()
{
  mPhysicsGroup = new QButtonGroup(this);
  QGroupBox *group = new QGroupBox("Physics");
  QGridLayout *layout = new QGridLayout();
  layout->setMargin(20);
  layout->setSpacing(0);

  group->setLayout(layout);
  return group;
}


QGroupBox* ControlInterface::makeRenderGroup()
{
  mRenderGroup = new QButtonGroup(this);
  QGroupBox *group = new QGroupBox("Render");
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setMargin(20);
  layout->setSpacing(10);
  
  // slider to adjust global daylight
  QLabel *sLabel = new QLabel("Light Level");
  QSlider *lSlider = new QSlider(Qt::Horizontal);
  lSlider->setTickInterval(1);
  lSlider->setTickPosition(QSlider::TicksBelow);
  lSlider->setMinimum(1);
  lSlider->setMaximum(mEngine->mLightLevels);
  lSlider->setValue(4);
  connect(lSlider, SIGNAL(sliderMoved(int)), this, SLOT(setLightLevel(int)));

  QHBoxLayout *lLayout = new QHBoxLayout();
  lLayout->addWidget(sLabel);
  lLayout->addWidget(lSlider);
  
  QCheckBox *rtCb = new QCheckBox("Ray Trace");
  connect(rtCb, SIGNAL(stateChanged(int)), this, SLOT(setRaytrace(int)));
  QCheckBox *wireCb = new QCheckBox("Wire Frame");
  connect(wireCb, SIGNAL(stateChanged(int)), this, SLOT(setWireframe(int)));
  QCheckBox *frustCb = new QCheckBox("Frustum Culling");
  connect(frustCb, SIGNAL(stateChanged(int)), this, SLOT(setFrustumCulling(int)));
  
  QGroupBox *rGroup = new QGroupBox("Render Distance");
  QGridLayout *rGrid = new QGridLayout();
  // Sliders to adjust chunk load radius
  mXSlider = new DisplaySlider("X Radius", 0, 16);
  mXSlider->setTickInterval(1);
  mXSlider->setLabelInterval(2);
  mXSlider->setValue(mWorld->getRadius()[0]);
  connect(mXSlider, SIGNAL(sliderMoved(int)), this, SLOT(setChunkRadiusX(int)));
  rGrid->addWidget(mXSlider,0,0,1,4);
  
  mYSlider = new DisplaySlider("Y Radius", 0, 16);
  mYSlider->setTickInterval(1);
  mYSlider->setLabelInterval(2);
  mYSlider->setValue(mWorld->getRadius()[0]);
  connect(mYSlider, SIGNAL(sliderMoved(int)), this, SLOT(setChunkRadiusY(int)));
  rGrid->addWidget(mYSlider,1,0,1,4);
  
  mZSlider = new DisplaySlider("Z Radius", 0, 8);
  mZSlider->setTickInterval(1);
  mZSlider->setLabelInterval(1);
  mZSlider->setValue(mWorld->getRadius()[2]);
  connect(mZSlider, SIGNAL(sliderMoved(int)), this, SLOT(setChunkRadiusZ(int)));
  rGrid->addWidget(mZSlider,2,0,1,4);

  rGroup->setLayout(rGrid);

  layout->addLayout(lLayout);
  layout->addWidget(rtCb);
  layout->addWidget(wireCb);
  layout->addWidget(frustCb);
  
  layout->addStretch(1);
  layout->addWidget(rGroup); // chunk radius settings at bottom of widget
  group->setLayout(layout);
  return group;
}
QGroupBox* ControlInterface::makeDebugGroup()
{
  mDebugGroup = new QButtonGroup(this);
  QGroupBox *group = new QGroupBox("Debug");
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setMargin(20);
  layout->setSpacing(0);
  
  QCheckBox *debugCb = new QCheckBox("Chunk Bounds");
  connect(debugCb, SIGNAL(stateChanged(int)), this, SLOT(setDebug(int)));
  
  layout->addWidget(debugCb);

  ChunkDisplay *loaded = new ChunkDisplay(mEngine->getWorld());

  layout->addWidget(loaded);
  layout->addStretch(1);
  
  group->setLayout(layout);
  return group;
}
