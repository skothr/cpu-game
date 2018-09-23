#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QWidget>
#include <string>

#include "vector.hpp"
#include "block.hpp"
#include "world.hpp"


class QVBoxLayout;
class QGridLayout;
class QStackedLayout;

class GameWidget;
class SystemMenu;
class VoxelEngine;
class WorldLoad;

class MainWindow : public QWidget
{
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  virtual ~MainWindow();

public slots:
  void createWorld(World::Options &options);
  void loadWorld(World::Options &options);
  void deleteWorld(std::string worldName);
protected slots:
  void selectMenu(int id);
  void quit();
  
protected:
  QSize minimumSizeHint() const;
  QSize sizeHint() const;
  
private:
  SystemMenu *mMenu = nullptr;
  GameWidget *mGame = nullptr;
  VoxelEngine *mEngine = nullptr;

  WorldLoad *mLoad = nullptr;
  
  int mMainMenuId;
  int mWorldCreateId;
  int mWorldLoadId;
  int mGameId;
};

#endif //MAIN_WINDOW_HPP
