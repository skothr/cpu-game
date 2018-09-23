#include "mainWindow.hpp"

#include "gameWidget.hpp"
#include "worldCreate.hpp"
#include "worldLoad.hpp"
#include "mainMenu.hpp"
#include "systemMenu.hpp"
#include "button.hpp"

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
  WorldCreate *create = new WorldCreate(this);
  mLoad = new WorldLoad(mEngine->getWorld(), this);

  mMenu = new SystemMenu(this);
  mMainMenuId = mMenu->addWidget(main);
  mWorldCreateId = mMenu->addWidget(create);
  mWorldLoadId = mMenu->addWidget(mLoad);
  mGameId = mMenu->addWidget(mGame);
  mMenu->changeMenu(mMainMenuId); // start at main menu

  // main
  connect(main, &MainMenu::createWorld, std::bind(&MainWindow::selectMenu, this, mWorldCreateId));
  connect(main, &MainMenu::loadWorld, std::bind(&MainWindow::selectMenu, this, mWorldLoadId));
  connect(main, &MainMenu::quit, std::bind(&QWidget::close, this));
  // create
  connect(create, &WorldCreate::created, this, &MainWindow::createWorld);
  connect(create, &WorldCreate::back, std::bind(&MainWindow::selectMenu, this, mMainMenuId));
  // load
  connect(mLoad, &WorldLoad::loaded, this, &MainWindow::loadWorld);
  connect(mLoad, &WorldLoad::deleted, this, &MainWindow::deleteWorld);
  connect(mLoad, &WorldLoad::back, std::bind(&MainWindow::selectMenu, this, mMainMenuId));
  // game
  connect(mGame, &GameWidget::quit, this, &QWidget::close);
  connect(mGame, &GameWidget::mainMenu, std::bind(&MainWindow::selectMenu, this, mMainMenuId));
  
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

void MainWindow::selectMenu(int id)
{
  if(id == mWorldLoadId)
    {
      mLoad->refreshList();
    }

  if(mMenu->currentMenu() == mGameId && id != mGameId)
    { // close pause menu
      mGame->pause(false);
    }
  
  mMenu->changeMenu(id);
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

void MainWindow::deleteWorld(std::string worldName)
{
  if(!mEngine->getWorld()->deleteWorld(worldName))
    { // TODO: If deleting fails, show message (?)
      LOGE("Could not delete world!");
    }
  //mLoad->refreshList();
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

