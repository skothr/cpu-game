#include "mainWindow.hpp"

#include "gameWidget.hpp"
#include "worldCreate.hpp"
#include "worldLoad.hpp"
#include "mainMenu.hpp"
#include "systemMenu.hpp"
#include "button.hpp"
#include "notifySlider.hpp"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>

#include <iostream>
#include <functional>

#define MARGIN 0
#define ITER_TICK_STEP 32
#define SAMPLE_TICK_STEP 1


MainWindow::MainWindow(QWidget *parent)
  : QWidget(parent)
{
  QPalette pal = palette();
  pal.setColor(QPalette::Button, QColor(Qt::black));
  setPalette(pal);
  update();
  
  // main game screen
  mGame = new GameWidget(this);
  mEngine = mGame->getEngine();
  
  // world create/load menus
  MainMenu *main = new MainMenu(this);
  WorldLoad *load = new WorldLoad(mEngine->getWorld(), this);
  WorldCreate *create = new WorldCreate(this);

  mMenu = new SystemMenu(this);
  mMainMenuId = mMenu->addWidget(main);
  mWorldCreateId = mMenu->addWidget(create);
  mWorldLoadId = mMenu->addWidget(load);
  mGameId = mMenu->addWidget(mGame);
  mMenu->changeMenu(mMainMenuId); // start at main menu

  // main
  connect(main, &MainMenu::createWorld, std::bind(&SystemMenu::changeMenu, mMenu, mWorldCreateId));
  connect(main, &MainMenu::loadWorld, std::bind(&SystemMenu::changeMenu, mMenu, mWorldLoadId));
  connect(main, &MainMenu::quit, std::bind(&QWidget::close, this));
  // create
  connect(create, &WorldCreate::created, this, &MainWindow::createWorld);
  connect(create, &WorldCreate::back, std::bind(&SystemMenu::changeMenu, mMenu, mMainMenuId));
  // load
  connect(load, &WorldLoad::loaded, this, &MainWindow::loadWorld);
  connect(load, &WorldLoad::back, std::bind(&SystemMenu::changeMenu, mMenu, mMainMenuId));
  // game
  connect(mGame, &GameWidget::quit, this, &QWidget::close);
  connect(mGame, &GameWidget::mainMenu, std::bind(&SystemMenu::changeMenu, mMenu, mMainMenuId));
  
  QHBoxLayout *mainLayout = new QHBoxLayout();
  mainLayout->addWidget(mMenu);
  setLayout(mainLayout);
  move(10,10);
  resize(960, 1020);
}

MainWindow::~MainWindow()
{
  LOGD("deconstructing mainwindow...");
}

void MainWindow::loadWorld(World::Options &options)
{
  if(mEngine->loadWorld(options))
    {
      mGame->start();
      LOGI("Switching to game menu...");
      mMenu->changeMenu(mGameId);
    }
  else
    { // TODO: Show popup dialog
      LOGE("Could not load world!");
    }
}
void MainWindow::createWorld(World::Options &options)
{
  if(mEngine->createWorld(options))
    {
      mGame->start();
      LOGI("Switching to game menu...");
      mMenu->changeMenu(mGameId);
    }
  else
    { // TODO: Show popup dialog
      LOGE("Could not create world!");
    }
}

void MainWindow::quit()
{
  // TODO: Popup to confirm quitting
  LOGI("Quitting to desktop...");
  close();
}

QSize MainWindow::minimumSizeHint() const
{ return QSize(512, 512); }

QSize MainWindow::sizeHint() const
{ return QSize(960, 1080); }


// void MainWindow::mousePressEvent(QMouseEvent *event)
// { }
// void MainWindow::mouseReleaseEvent(QMouseEvent *event)
// { }
// void MainWindow::mouseMoveEvent(QMouseEvent *event)
// { }
// void MainWindow::keyPressEvent(QKeyEvent *event)
// { }
// void MainWindow::keyReleaseEvent(QKeyEvent *event)
// { }
// void MainWindow::wheelEvent(QWheelEvent *event)
// { }
