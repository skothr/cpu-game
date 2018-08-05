#ifndef OVERLAY_HPP
#define OVERLAY_HPP

#include <QWidget>
#include <vector>
#include "vector.hpp"
#include "helpers.hpp"

class QLabel;
class QVBoxLayout;

enum align_t
  {
   NONE = 0x00,
   LEFT = 0x01,
   RIGHT = 0x02,
   TOP = 0x04,
   BOTTOM = 0x08,
   HCENTER = 0x10,
   VCENTER = 0x20,
  };


struct LabelDesc
{
  LabelDesc() {}
  LabelDesc(const std::string &text, align_t align)
    : text(text), alignment(align)
  { }
  std::string text;
  align_t alignment;
};


class cOverlay : public QWidget
{
  Q_OBJECT
public:
  cOverlay(QWidget *parent, const std::vector<LabelDesc> &labels);

  QLabel* getLabel(int i);
  
private:
  QVBoxLayout *mLayout = nullptr;
  std::vector<QLabel*> mLabels;
};


#endif // OVERLAY_HPP
