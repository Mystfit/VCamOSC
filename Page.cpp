#include "Page.h"
#include <algorithm> 

Page::Page(const char* name) : 
  m_name(name),
  m_hovered_item_idx(0){
}

void Page::AddItem(std::shared_ptr<Item> item){
  // If empty, select first added item
  if(!m_items.size()){
    item->SetSelected(true);
    m_hovered_item_idx = 0;
  }
  m_items.push_back(item);
}

std::shared_ptr<Item> Page::GetSelectedItem(){
  if(m_hovered_item_idx < m_items.size())
    return m_items[m_hovered_item_idx];
  return nullptr;
}

void Page::SelectItemAtIndex(uint16_t index){
  // Bounds check for selection
  index = std::min(size_t(index), m_items.size() - 1);
  
  // Reset existing selected item
  if(m_hovered_item_idx < m_items.size())
    m_items[m_hovered_item_idx]->SetSelected(false);
  
  // Set new selected item  
  m_items[index]->SetSelected(true);
  m_hovered_item_idx = index;
}

uint16_t Page::GetSelectedItemIndex(){
  return m_hovered_item_idx;
}

const char* Page::GetName(){
  return m_name.c_str();
}

size_t Page::GetNumItems(){
  return m_items.size();
}

bool Page::ScrollForwards(){
  if(m_hovered_item_idx < m_items.size()){
    if(m_items[m_hovered_item_idx]->ScrollForwards()){
      return true;
    }
  }
  
  Serial.println("Scrolling page selection forwards");
  SelectItemAtIndex((m_hovered_item_idx + 1) % m_items.size());
  return true;
}

bool Page::ScrollBackwards(){
  if(m_hovered_item_idx < m_items.size()){
    if(m_items[m_hovered_item_idx]->ScrollBackwards()){
      return true;
    }
  }
  
  Serial.println("Scrolling page selection backwards");
  SelectItemAtIndex((m_hovered_item_idx-1) < 0 ? m_items.size()-1 : m_hovered_item_idx-1);
  return true;
}

bool Page::HandleSelect(){
  if(m_hovered_item_idx < m_items.size()){
    return m_items[m_hovered_item_idx]->HandleSelect();
  }
  return false;
}

bool Page::HandleBack(){
  if(m_hovered_item_idx < m_items.size()){
    return m_items[m_hovered_item_idx]->HandleBack();
  }
  return false;
}

bool Page::HandleDoubleClick(){
  return false;
}

void Page::draw(Adafruit_SH1107& display){
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextWrap(false);
  
  // Name of page is the title
  display.setTextColor(SH110X_WHITE, SH110X_BLACK);
  display.println(GetName());
  
  // Let all items draw themselves
  for(auto item : m_items){
    item->draw(display);
  }
  
  display.display();
}
