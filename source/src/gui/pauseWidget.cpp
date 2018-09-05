#include "pauseWidget.hpp"
#include "button.hpp"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QString>

PauseWidget::PauseWidget(QWidget *parent)
  : QWidget(parent)
{
  QVBoxLayout *vb = new QVBoxLayout();
  vb->setSpacing(0);

  Button *resumeBtn = new Button("Resume");
  Button *quitBtn = new Button("Quit");
  connect(quitBtn, SIGNAL(clicked()), this, SIGNAL(quit()));
  connect(resumeBtn, SIGNAL(clicked()), this, SIGNAL(resumed()));

  vb->addStretch(1);
  vb->addWidget(resumeBtn);
  vb->addWidget(quitBtn);
  vb->addStretch(1);

  QHBoxLayout *hb = new QHBoxLayout(this);
  hb->addStretch(1);
  hb->addLayout(vb);
  hb->addStretch(1);
  setLayout(hb);
}
