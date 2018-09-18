#ifndef DISPLAY_SLIDER_HPP
#define DISPLAY_SLIDER_HPP

#include <QWidget>
#include <vector>

class QLabel;
class QSlider;
class QGridLayout;

class DisplaySlider : public QWidget
{
  Q_OBJECT
public:
  DisplaySlider(const std::string &label, int minVal, int maxVal, QWidget *parent = nullptr);
  virtual ~DisplaySlider();

  void setTickInterval(int interval);
  void setLabelInterval(int interval);
  void setValue(int val);
  
signals:
  void sliderMoved(int val);
protected slots:
  void sliderChanged(int val);
private:
  QSlider *mSlider = nullptr;
  QLabel  *mValLabel = nullptr;
  QGridLayout *mLayout = nullptr;

  std::string mLabel;

  int mLabelInterval = 0;
  int mMin = 0;
  int mMax = 0;
  std::vector<QLabel*> mTickLabels;
};

#endif // DISPLAY_SLIDER_HPP
