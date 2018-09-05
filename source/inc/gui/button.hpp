#ifndef BUTTON_HPP
#define BUTTON_HPP

#include <QWidget>

class Button : public QWidget
{
  Q_OBJECT
public:
  Button(const std::string &text, int btnId = -1, QWidget *parent = nullptr);
  virtual ~Button() { }

signals:
  void clicked(int id = -1);
  
protected slots:
  void btnClicked(bool checked);
private:
  int mId;
};

#endif // BUTTON_HPP
