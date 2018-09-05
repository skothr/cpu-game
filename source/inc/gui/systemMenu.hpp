#ifndef SYSTEM_MENU_HPP
#define SYSTEM_MENU_HPP

#include "button.hpp"
#include <QWidget>

class QStackedLayout;

class SystemMenu : public QWidget
{
  Q_OBJECT
public:
  SystemMenu(QWidget *parent = nullptr);
  virtual ~SystemMenu() { }

  struct Menu
  {
    std::string title = "";
    std::vector<Button*> buttons;
  };
  int addMenu(const Menu &menu);
  int addWidget(QWidget *widget);

signals:
  void menuChanged(int id);
public slots:
  void changeMenu(int id);
  
private:
  QStackedLayout *mStack = nullptr;
};



#endif // SYSTEM_MENU_HPP
