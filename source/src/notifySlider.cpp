#include "notifySlider.hpp"

#include <QWidget>


NotifySlider::NotifySlider(QWidget *parent, double *notify, int numTicks)
  : QSlider(Qt::Horizontal, parent), mNotify(notify), mNumTicks(numTicks)
{
  setTickInterval(mNumTicks);
}

NotifySlider::~NotifySlider()
{
  if(mNotify)
    { *mNotify = 0.0; }
}


void NotifySlider::valueChanged(int value)
{
  if(mNotify)
    { *mNotify = (double)value / (double)mNumTicks; }
}
