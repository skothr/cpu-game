#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QtGui>
#include <QWidget>

#include "vector.hpp"

class QVBoxLayout;
class QGridLayout;
class QPushButton;
class QSpinBox;
class cGameWidget;
class QLabel;
class cOverlay;
class QStackedLayout;

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
  
protected:
  void keyPressEvent(QKeyEvent *event);
  
private:
  cGameWidget *mGame = nullptr;
  QStackedLayout *mMainLayout = nullptr;
  cOverlay *mOverlay = nullptr;
};

#endif //MAIN_WINDOW_HPP
