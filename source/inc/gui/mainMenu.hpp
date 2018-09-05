#ifndef MAIN_MENU_HPP
#define MAIN_MENU_HPP

#include <QWidget>

class MainMenu : public QWidget
{
  Q_OBJECT
public:
  MainMenu(QWidget *parent = nullptr);
  virtual ~MainMenu() { }

signals:
  void createWorld();
  void loadWorld();
  void quit();
  
private:
};

#endif // MAIN_MENU_HPP
