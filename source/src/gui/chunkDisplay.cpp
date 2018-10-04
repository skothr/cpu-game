#include "chunkDisplay.hpp"
#include "chunkLoadWidget.hpp"
#include "displaySlider.hpp"

#include <QGridLayout>
#include <QCheckBox>
#include <functional>


ChunkDisplay::ChunkDisplay(World *world, QWidget *parent)
  : QWidget(parent)
{
  mChunkWidget = new ChunkLoadWidget(world, this);

  QCheckBox *centerCB  = new QCheckBox("Center (blue)");
  QCheckBox *renderCB  = new QCheckBox("Rendered (white)");
  QCheckBox *emptyCB   = new QCheckBox("Empty (grey)");
  QCheckBox *meshedCB  = new QCheckBox("Meshed (green)");
  QCheckBox *readyCB   = new QCheckBox("Neighbors Ready (dark green)");
  QCheckBox *loadedCB  = new QCheckBox("Loaded (dark cyan)");
  QCheckBox *loadingCB = new QCheckBox("Loading (yellow)");
  
  QPalette p = centerCB->palette();
  p.setColor(QPalette::Active, QPalette::WindowText, Qt::blue);
  centerCB->setPalette(p);
  p.setColor(QPalette::Active, QPalette::WindowText, Qt::white);
  renderCB->setPalette(p);
  p.setColor(QPalette::Active, QPalette::WindowText, Qt::gray);
  emptyCB->setPalette(p);
  p.setColor(QPalette::Active, QPalette::WindowText, Qt::green);
  meshedCB->setPalette(p);
  p.setColor(QPalette::Active, QPalette::WindowText, Qt::darkGreen);
  readyCB->setPalette(p);
  p.setColor(QPalette::Active, QPalette::WindowText, Qt::darkCyan);
  loadedCB->setPalette(p);
  p.setColor(QPalette::Active, QPalette::WindowText, Qt::yellow);
  loadingCB->setPalette(p);
  
  centerCB->setCheckState(mChunkWidget->mOptions[0] ? Qt::Checked : Qt::Unchecked);
  renderCB->setCheckState(mChunkWidget->mOptions[1] ? Qt::Checked : Qt::Unchecked);
  emptyCB->setCheckState(mChunkWidget->mOptions[2] ? Qt::Checked : Qt::Unchecked);
  meshedCB->setCheckState(mChunkWidget->mOptions[3] ? Qt::Checked : Qt::Unchecked);
  readyCB->setCheckState(mChunkWidget->mOptions[4] ? Qt::Checked : Qt::Unchecked);
  loadedCB->setCheckState(mChunkWidget->mOptions[5] ? Qt::Checked : Qt::Unchecked);
  loadingCB->setCheckState(mChunkWidget->mOptions[6] ? Qt::Checked : Qt::Unchecked);

  connect(centerCB, &QCheckBox::stateChanged,
          std::bind(&ChunkLoadWidget::setOption, mChunkWidget,
                    ChunkLoadWidget::OPT_CENTER, std::placeholders::_1 ));
  connect(renderCB, &QCheckBox::stateChanged,
          std::bind(&ChunkLoadWidget::setOption, mChunkWidget,
                    ChunkLoadWidget::OPT_RENDER, std::placeholders::_1 ));
  connect(emptyCB, &QCheckBox::stateChanged,
          std::bind(&ChunkLoadWidget::setOption, mChunkWidget,
                    ChunkLoadWidget::OPT_EMPTY, std::placeholders::_1 ));
  connect(meshedCB, &QCheckBox::stateChanged,
          std::bind(&ChunkLoadWidget::setOption, mChunkWidget,
                    ChunkLoadWidget::OPT_MESHED, std::placeholders::_1 ));
  connect(readyCB, &QCheckBox::stateChanged,
          std::bind(&ChunkLoadWidget::setOption, mChunkWidget,
                    ChunkLoadWidget::OPT_READY, std::placeholders::_1 ));
  connect(loadedCB, &QCheckBox::stateChanged,
          std::bind(&ChunkLoadWidget::setOption, mChunkWidget,
                    ChunkLoadWidget::OPT_LOADED, std::placeholders::_1 ));
  connect(loadingCB, &QCheckBox::stateChanged,
          std::bind(&ChunkLoadWidget::setOption, mChunkWidget,
                    ChunkLoadWidget::OPT_LOADING, std::placeholders::_1 ));

  DisplaySlider *zSlider = new DisplaySlider("Z OFFSET", -4, 4, this);
  zSlider->setTickInterval(1);
  zSlider->setLabelInterval(1);
  connect(zSlider, &DisplaySlider::sliderMoved,
          std::bind(&ChunkLoadWidget::setZ, mChunkWidget, std::placeholders::_1) );
  
  QGridLayout *layout = new QGridLayout();
  layout->addWidget(centerCB, 0,0,1,1);
  layout->addWidget(renderCB, 0,1,1,1);
  layout->addWidget(emptyCB, 1,0,1,1);
  layout->addWidget(meshedCB, 1,1,1,1);
  layout->addWidget(readyCB, 2,0,1,1);
  layout->addWidget(loadedCB, 2,1,1,1);
  layout->addWidget(loadingCB, 3,0,1,1);

  layout->addWidget(zSlider, 4,0,1,1);
  layout->addWidget(mChunkWidget, 5,0,1,1);
  
  setLayout(layout);
}
