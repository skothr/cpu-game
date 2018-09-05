#include "mainMenu.hpp"

#include "button.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>

MainMenu::MainMenu(QWidget *parent)
  : QWidget(parent)
{
  Button *loadBtn = new Button("Load World");
  Button *createBtn = new Button("Create New World");
  Button *quitBtn = new Button("Quit to Desktop");
  connect(loadBtn, SIGNAL(clicked()), this, SIGNAL(loadWorld()));
  connect(createBtn, SIGNAL(clicked()), this, SIGNAL(createWorld()));
  connect(quitBtn, SIGNAL(clicked()), this, SIGNAL(quit()));

  QVBoxLayout *vb = new QVBoxLayout();
  vb->addStretch(1);
  vb->addWidget(loadBtn);
  vb->addWidget(createBtn);
  vb->addWidget(quitBtn);
  vb->addStretch(1);

  QHBoxLayout *hb = new QHBoxLayout();
  hb->addStretch(1);
  hb->addLayout(vb);
  hb->addStretch(1);
  setLayout(hb);
}
