#ifndef NOTIFY_SLIDER_HPP
#define NOTIFY_SLIDER_HPP

#include <QSlider>

class NotifySlider : public QSlider
{
public:
  NotifySlider(QWidget *parent = nullptr, double *notify = nullptr, int numTicks = 100);
  ~NotifySlider();

  void valueChanged(int value);
  
private:
  double *mNotify = nullptr;
  int mNumTicks = 100;
};

#endif //NOTIFY_SLIDER_HPP
