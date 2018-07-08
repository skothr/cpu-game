#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QtGui>
#include <QWidget>

#include "vector.hpp"

class QFormLayout;
class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QSpinBox;
class cGameWidget;

class cMainWindow : public QWidget
{
  Q_OBJECT
public:
  explicit cMainWindow(QWidget *parent = nullptr);
  virtual ~cMainWindow();

public slots:
  void togglePhysics(bool checked);
  void stepPhysics();
  void setTimestep(int timestepMs);
protected:
  void keyPressEvent(QKeyEvent *event);
  
private:
  cGameWidget *mGame = nullptr;
  QVBoxLayout *mMainLayout = nullptr;
  QFormLayout *mForm = nullptr;
};

#endif //MAIN_WINDOW_HPP
