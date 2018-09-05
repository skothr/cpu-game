#include "button.hpp"
#include <QPushButton>
#include <QVBoxLayout>

Button::Button(const std::string &text, int btnId, QWidget *parent)
  : QWidget(parent), mId(btnId)
{
  QPushButton *btn = new QPushButton(QString(text.c_str()));
  btn->setFlat(true);
  btn->setAutoFillBackground(true);
  connect(btn, SIGNAL(clicked(bool)), this, SLOT(btnClicked(bool)));
  QVBoxLayout *vb = new QVBoxLayout();
  vb->addWidget(btn);
  setLayout(vb);
}

void Button::btnClicked(bool checked)
{
  emit clicked(mId);
}
