#ifndef POPUP_HPP
#define POPUP_HPP

#include "button.hpp"

class Popup : public QWidget
{
public:
  Popup(const std::string &text, const std::vector<Button*> &buttons);
  virtual ~Popup() { }

private:
  
};

#endif // POPUP_HPP
