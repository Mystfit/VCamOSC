#pragma once

#include <Adafruit_SH110X.h>
#include <vector>
#include <memory>
#include <stack>
#include "Page.h"


class OLEDMenu {
public:
  OLEDMenu();
  void AddPage(std::shared_ptr<Page> page);
  void PushPage(std::shared_ptr<Page> page);
  void PopPage();
  void ScrollForwards();
  void ScrollBackwards();
  void HandleButton(uint8_t event);
  void HandleButtonSelect();
  void HandleButtonBack();
  void HandleButtonDoubleClick();

  void draw(Adafruit_SH1107& display);
  
private:
  std::vector<std::shared_ptr<Page> > m_pages;
  std::stack<std::shared_ptr<Page> > m_page_stack;
};

