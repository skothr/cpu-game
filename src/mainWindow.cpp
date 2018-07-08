#include "mainWindow.hpp"

#include "gameWidget.hpp"
#include "notifySlider.hpp"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpinBox>

#include <iostream>

#define MARGIN 0
#define ITER_TICK_STEP 32
#define SAMPLE_TICK_STEP 1


cMainWindow::cMainWindow(QWidget *parent)
  : QWidget(parent)
{
  mGame = new cGameWidget(this);
  
  // LAYOUT
  mMainLayout = new QVBoxLayout(this);
  mMainLayout->setMargin(MARGIN);
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
