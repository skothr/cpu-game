#include "overlay.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QFont>
#include <iostream>

Overlay::Overlay(QWidget *parent, const std::vector<LabelDesc> &labels)
  : QWidget(parent)
{
  mLayout = new QVBoxLayout(this);
  //

  QFont f("Arial", 12, QFont::Bold);
  
  for(auto &l : labels)
    {
      Qt::Alignment align = (((l.alignment & align_t::LEFT) ? Qt::AlignLeft : (Qt::AlignmentFlag)0) |
                             ((l.alignment & align_t::RIGHT) ? Qt::AlignRight : (Qt::AlignmentFlag)0) |
                             ((l.alignment & align_t::TOP) ? Qt::AlignTop : (Qt::AlignmentFlag)0) |
                             ((l.alignment & align_t::BOTTOM) ? Qt::AlignBottom : (Qt::AlignmentFlag)0) |
                             ((l.alignment & align_t::HCENTER) ? Qt::AlignHCenter : (Qt::AlignmentFlag)0) |
                             ((l.alignment & align_t::VCENTER) ? Qt::AlignVCenter : (Qt::AlignmentFlag)0) );
      
      mLabels.push_back(new QLabel(l.text.c_str(), this));
      mLabels.back()->setFont(f);
      //mLabels.back()->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
      mLayout->addWidget(mLabels.back()); //align);
      mLayout->setAlignment(align);
    }
  setLayout(mLayout);
  setAttribute(Qt::WA_TransparentForMouseEvents);
}


QLabel* Overlay::getLabel(int i)
{
  return mLabels[i];
}
