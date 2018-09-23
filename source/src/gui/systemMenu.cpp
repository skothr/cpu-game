#include "systemMenu.hpp"

#include <QStackedLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QLabel>

SystemMenu::SystemMenu(QWidget *parent)
  : QWidget(parent)
{
  mStack = new QStackedLayout(this);
  //connect(this, SIGNAL(menuChanged(int)), mStack, SLOT(setCurrentIndex(int)));
  setLayout(mStack);
}

int SystemMenu::currentMenu() const
{ return mStack->currentIndex(); }

void SystemMenu::changeMenu(int id)
{
  mStack->setCurrentIndex(id);
}

int SystemMenu::addMenu(const Menu &menu)
{
  QWidget *mWidget = new QWidget(this);
  QHBoxLayout *mhLayout = new QHBoxLayout();
  QVBoxLayout *mvLayout = new QVBoxLayout();

  mvLayout->setAlignment(Qt::AlignVCenter);

  mvLayout->addWidget(new QLabel(QString(menu.title.c_str())));
  //mvLayout->addStretch(1);
  for(auto b : menu.buttons)
    { mvLayout->addWidget(b); }
  //mvLayout->addStretch(1);

  mhLayout->addStretch(1);
  mhLayout->addLayout(mvLayout);
  mhLayout->addStretch(1);

  mWidget->setLayout(mhLayout);  
  mStack->addWidget(mWidget);
  return mStack->count()-1;
}

int SystemMenu::addWidget(QWidget *widget)
{
  mStack->addWidget(widget);
  return mStack->count()-1;
}
