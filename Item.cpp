#include "Item.h"
#include "Page.h"
#include <Adafruit_GFX.h>


Item::Item(const char* name, std::shared_ptr<Page> page, bool selectable, CallEvent called_cb) : 
    m_name(name), 
    m_selected(false),
    m_selectable(selectable),
    m_target(page),
    m_called_cb(called_cb)
{
}

void Item::SetSelected(bool state){
  if(m_selectable){
    m_selected = state;
  }
}

bool Item::IsSelected(){
  return m_selected;
}

bool Item::IsSelectable(){
  return m_selectable;
}

const char* Item::GetName(){
  return m_name.c_str();
}

std::shared_ptr<Page> Item::GetTarget(){
  return m_target;
}

void Item::operator()(){
  HandleSelect();
}

bool Item::ScrollForwards(){
  return false;
}

bool Item::ScrollBackwards(){
  return false;
}

bool Item::HandleSelect(){
  if(m_called_cb){
    m_called_cb();
    return true;
  } else {
    Serial.println("Item has no registered cb");
  }
  return false;
}

bool Item::HandleBack(){
  return false;
}

void Item::draw(Adafruit_SH1107& display){
  display.setTextColor(SH110X_WHITE, SH110X_BLACK);
  
  if(IsSelectable()){
    if(this->IsSelected()){
        display.setTextColor(SH110X_BLACK, SH110X_WHITE);
        display.print(">");
    } else {
        display.setTextColor(SH110X_WHITE, SH110X_BLACK);
        display.print(" ");
    }
  }
  display.println(this->GetName());
}