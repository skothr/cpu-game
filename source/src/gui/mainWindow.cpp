#include "mainWindow.hpp"

#include "gameWidget.hpp"
#include "notifySlider.hpp"
#include "overlay.hpp"
#include "controlInterface.hpp"

#include <QStackedLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QRadioButton>

#include <iostream>

#define MARGIN 0
#define ITER_TICK_STEP 32
#define SAMPLE_TICK_STEP 1


cMainWindow::cMainWindow(QWidget *parent, int numThreads, const std::string &worldName, uint32_t seed)
  : QWidget(parent)
{
  mGame = new cGameWidget(this, numThreads, worldName, seed);
  connect(mGame, SIGNAL(posChanged(Point3f, Point3i, Point3i)),
          this, SLOT(setPosition(Point3f, Point3i, Point3i)));
  connect(mGame, SIGNAL(blockInfo(block_t, int)),
          this, SLOT(setBlockInfo(block_t, int)));
  

  mOverlay = new cOverlay(this, {LabelDesc("POSITION", (align_t)(align_t::TOP)),
                                 LabelDesc("CHUNK", (align_t)(align_t::TOP)),
                                 LabelDesc("BLOCK", (align_t)(align_t::TOP)),
                                 LabelDesc("LIGHT", (align_t)(align_t::TOP))});

  mOverlay->raise();
  //mChunkLabel->raise();
  
  // game viewport layout
  mGameLayout = new QStackedLayout();
  mGameLayout->setStackingMode(QStackedLayout::StackAll);
  //mMainLayout->setMargin(MARGIN);
  mGameLayout->addWidget(mOverlay);
  mGameLayout->addWidget(mGame);

  // control layouts
  mBottomControl = new cControlInterface(mGame->getEngine());

  // add to main grid layout
  mMainLayout = new QGridLayout(this);
  mMainLayout->setSpacing(0);
  mMainLayout->setMargin(0);
  mMainLayout->addLayout(mGameLayout, 0, 0, 4, 4);
  mMainLayout->addWidget(mBottomControl, 2, 0, 2, 4, Qt::AlignBottom);
  
  setLayout(mMainLayout);
}

cMainWindow::~cMainWindow()
{
  
}
void cMainWindow::setTimestep(int timestepMs)
{
  //mGame->setTimestep(timestepMs);
}

void cMainWindow::stepPhysics()
{
  //mGame->stepPhysics(1);
}

void cMainWindow::togglePhysics(bool checked)
{
  //mPhysicsBtn->setText(checked ? "Stop Physics ><" : "Start Physics >>");
  //mGame->togglePhysics(checked);
}

void cMainWindow::setPosition(Point3f player, Point3i collisions, Point3i chunk)
{
  //std::cout << "SETTING POSITION: " << player << ", chunk: " << chunk << "\n";
  QLabel *pl = mOverlay->getLabel(0);
  QLabel *cl = mOverlay->getLabel(1);
  std::stringstream ss;
  ss << "POSITION:  " << player << "  (" << collisions << ")";
  pl->setText(ss.str().c_str());
  ss.str("");
  ss.clear();
  ss << "CHUNK:     " << chunk;
  cl->setText(ss.str().c_str());
}

void cMainWindow::setBlockInfo(block_t type, int lightLevel)
{
  QLabel *tl = mOverlay->getLabel(2);
  QLabel *ll = mOverlay->getLabel(3);
  tl->setText(("BLOCK TYPE: " + toString(type)).c_str());
  std::stringstream ss;
  ss << "LIGHT LEVEL:     " << lightLevel;
  ll->setText(ss.str().c_str());
}

void cMainWindow::keyPressEvent(QKeyEvent *event)
{
  switch(event->key())
    {
    case Qt::Key_Escape:   // quit program
      LOGI("Escape pressed (cMainWindow).");
      close();
      event->accept();
      break;
    case Qt::Key_X:   // toggle mouse capture
      mGame->captureMouse(!mGame->getMouseCaptured());
      if(mGame->getMouseCaptured())
        {
          LOGI("Captured mouse!");
          mBottomControl->collapse();
        }
      else
        {
          LOGI("Released mouse!");
          mBottomControl->expand();
        }
      event->accept();
      break;

    default:
      event->ignore();
      break;
    }
}

void cMainWindow::toolSelected(int id, bool checked)
{
  if(checked)
    {
      mGame->setTool((block_t)id);
    }
}

    
