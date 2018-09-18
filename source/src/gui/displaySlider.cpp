#include  "displaySlider.hpp"

#include <QLabel>
#include <QSlider>
#include <QGridLayout>

DisplaySlider::DisplaySlider(const std::string &label, int minVal, int maxVal, QWidget *parent)
  : QWidget(parent), mLabel(label), mMin(minVal), mMax(maxVal)
{
  mLayout = new QGridLayout();
  mValLabel = new QLabel(QString(mLabel.c_str()) + ":  " + QString::number(mMin));
  mSlider = new QSlider(Qt::Horizontal);
  
  mSlider->setTickInterval(1);
  mSlider->setTickPosition(QSlider::TicksBelow);
  mSlider->setMinimum(mMin);
  mSlider->setMaximum(mMax);

  //setLabelInterval((mMax - mMin) / 4);
  
  connect(mSlider, SIGNAL(sliderMoved(int)), this, SLOT(sliderChanged(int)));
}
DisplaySlider::~DisplaySlider()
{

}

void DisplaySlider::setValue(int val)
{
  mSlider->setValue(val);
  mValLabel->setText(QString(mLabel.c_str()) + ":  " + QString::number(val));
  emit sliderMoved(val);
}

void DisplaySlider::sliderChanged(int val)
{
  mValLabel->setText(QString(mLabel.c_str()) + ":  " + QString::number(val));
  emit sliderMoved(val);
}


void DisplaySlider::setTickInterval(int interval)
{
  mSlider->setTickInterval(interval);
}

void DisplaySlider::setLabelInterval(int interval)
{
  mLabelInterval = interval;


  if(mLayout)
    { delete mLayout; }
  mLayout = new QGridLayout();
  mLayout->addWidget(mValLabel, 0,0,1,1);
  mLayout->addWidget(mSlider, 0,1,1,(mMax - mMin) / interval+1);
  
  mTickLabels.clear();
  if(interval < 0)
    { // no labels
      
    }
  else if(interval >= 0 && interval <= mMax - mMin)
    { // min and max
      int numCenter = (mMax - mMin) / interval - 1;
      QLabel *l = new QLabel(QString::number(mMin));
      mTickLabels.push_back(l);
      mLayout->addWidget(l, 1,1,1,1, Qt::AlignHCenter);
      int i;
      for(i = 1; i <= numCenter; i++)
        {
          l = new QLabel(QString::number(mMin + i*interval));
          mLayout->addWidget(l, 1,1+i,1,1, Qt::AlignHCenter);
          mTickLabels.push_back(l);
        }
      l = new QLabel(QString::number(mMax));
      mTickLabels.push_back(l);
      mLayout->addWidget(l, 1,1+i,1,1, Qt::AlignHCenter);
    }

  setLayout(mLayout);
}
