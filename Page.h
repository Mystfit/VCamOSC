#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <vector>
#include "Item.h"

class Page {
public:
  Page(const char* name);
  void AddItem(std::shared_ptr<Item> item);
  std::shared_ptr<Item> GetSelectedItem();
  void SelectItemAtIndex(uint16_t index);
  uint16_t GetSelectedItemIndex();
  const char* GetName();
  size_t GetNumItems();
  
  virtual bool ScrollForwards();
  virtual bool ScrollBackwards();
  virtual bool HandleSelect();
  virtual bool HandleBack();
  virtual bool HandleDoubleClick();
  
  virtual void draw(Adafruit_SH1107& display);

private:
  std::string m_name;
  std::vector<std::shared_ptr<Item> > m_items;
  uint16_t m_hovered_item_idx;
};
