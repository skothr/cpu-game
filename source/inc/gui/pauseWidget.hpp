#ifndef PAUSE_WIDGET_HPP
#define PAUSE_WIDGET_HPP

#include <QWidget>

class PauseWidget : public QWidget
{
  Q_OBJECT
public:
  PauseWidget(QWidget *parent = nullptr);
  virtual ~PauseWidget() { }
  
signals:
  void resumed();
  void mainMenu();
  void quit();
  
protected:
  
};

#endif // PAUSE_WIDGET_HPP
