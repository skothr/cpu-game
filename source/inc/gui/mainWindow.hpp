#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QWidget>

#include "vector.hpp"
#include "block.hpp"
#include "world.hpp"

class QVBoxLayout;
class QGridLayout;
class QStackedLayout;

class GameWidget;
class SystemMenu;
class VoxelEngine;

class MainWindow : public QWidget
{
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  virtual ~MainWindow();

public slots:
  void createWorld(const World::Options &options);
  void loadWorld(const World::Options &options);
protected slots:
  void quit();
  
protected:
  QSize minimumSizeHint() const;
  QSize sizeHint() const;
  
private:
  SystemMenu *mMenu = nullptr;
  GameWidget *mGame = nullptr;
  VoxelEngine *mEngine = nullptr;

  int mMainMenuId;
  int mWorldCreateId;
  int mWorldLoadId;
  int mGameId;
};

#endif //MAIN_WINDOW_HPP
