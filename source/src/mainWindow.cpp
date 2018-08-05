#include "mainWindow.hpp"

#include "gameWidget.hpp"
#include "notifySlider.hpp"
#include "overlay.hpp"

#include <QGridLayout>
#include <QVBoxLayout>
#include <QLabel>

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

  mOverlay = new cOverlay(this, {LabelDesc("POSITION", (align_t)(align_t::TOP)),
                                 LabelDesc("CHUNK", (align_t)(align_t::TOP))});

  mOverlay->raise();
  //mChunkLabel->raise();
  
  // LAYOUT
  mMainLayout = new QStackedLayout(this);
  mMainLayout->setStackingMode(QStackedLayout::StackAll);
  //mMainLayout->setMargin(MARGIN);
  mMainLayout->addWidget(mOverlay);
  mMainLayout->addWidget(mGame);
  
  setLayout(mMainLayout);
}

cMainWindow::~cMainWindow()
{
  std::cout << "~cMainWindow()...\n";
  /*
  LOGD("deleting game...");
  if(mGame)
    { delete mGame; }
  LOGD("deleting main layout...");
  if(mMainLayout)
    { delete mMainLayout; }
  */
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

void cMainWindow::keyPressEvent(QKeyEvent *event)
{
  switch(event->key())
    {
    case Qt::Key_Escape:   // Quit propgram.
      LOGI("Escape pressed (cMainWindow).");
      close();
      LOGI("Window closed.");
      break;

    default:
      break;
    }
}
