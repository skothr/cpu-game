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
#include <QLabel>


cControlInterface::cControlInterface(cVoxelEngine *engine, QWidget *parent)
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

  mTabStack->addWidget(mMaterialWidget);
  mTabStack->addWidget(mPhysicsWidget);
  mTabStack->addWidget(mLightWidget);
  
  mControlTabs->insertTab(0, QString("Materials"));
  mControlTabs->insertTab(1, QString("Physics"));
  mControlTabs->insertTab(2, QString("Lighting"));
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

QGroupBox* cControlInterface::makeSimpleGroup()
{
  mSimpleGroup = new QButtonGroup(this);
  QGroupBox *group = new QGroupBox("Material Select");
  QVBoxLayout *vbox = new QVBoxLayout();

  for(int b = (int)block_t::NONE; b < (int)block_t::COMPLEX_START; b++)
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
  QGroupBox *group = new QGroupBox("Material Select");
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
  QGroupBox *group = new QGroupBox("Material Select");
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
