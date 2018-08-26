#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QtGui>
#include <QWidget>

#include "vector.hpp"
#include "block.hpp"

class QVBoxLayout;
class QGridLayout;
class cGameWidget;
class cOverlay;
class QStackedLayout;
class cControlInterface;

class cMainWindow : public QWidget
{
  Q_OBJECT
public:
  explicit cMainWindow(QWidget *parent = nullptr, int numThreads = 1,
		       const std::string &worldName = "", uint32_t seed = 0 );
  virtual ~cMainWindow();
			
public slots:
  void togglePhysics(bool checked);
  void stepPhysics();
  void setTimestep(int timestepMs);
  void setPosition(Point3f player, Point3i collisions, Point3i chunk);
  void setBlockInfo(block_t type, int lightLevel);
  
protected slots:
  void toolSelected(int id, bool checked);
  
protected:
  void keyPressEvent(QKeyEvent *event);

private:
  cGameWidget *mGame = nullptr;
  QStackedLayout *mGameLayout = nullptr;
  cOverlay *mOverlay = nullptr;
  cControlInterface *mBottomControl = nullptr;
  
  QGridLayout *mMainLayout = nullptr;
};

#endif //MAIN_WINDOW_HPP
