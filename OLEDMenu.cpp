#include "OLEDMenu.h"
#include <Adafruit_GFX.h>
#include <AceButton.h>

using namespace ace_button;


OLEDMenu::OLEDMenu(){
}

void OLEDMenu::AddPage(std::shared_ptr<Page> page){
    m_pages.push_back(page);
}

void OLEDMenu::PushPage(std::shared_ptr<Page> page){
    m_page_stack.push(page);
}

void OLEDMenu::PopPage(){
    m_page_stack.pop();
}

void OLEDMenu::ScrollForwards(){
  if(auto page = m_page_stack.top()){
    page->ScrollForwards();
  }
}

void OLEDMenu::ScrollBackwards(){
  if(auto page = m_page_stack.top()){
    page->ScrollBackwards();
  }
}

void OLEDMenu::HandleButton(uint8_t event){
  switch (event) {
    case AceButton::kEventPressed:
      Serial.println("Menu enc press");
      break;
    case AceButton::kEventReleased:
      Serial.println("Menu enc release");
      HandleButtonSelect();
      break;
    case AceButton::kEventDoubleClicked:
      Serial.println("Menu enc double-click");
      HandleButtonDoubleClick();
      break;
    case AceButton::kEventLongPressed:
      Serial.println("Menu enc long-press");
      HandleButtonBack();
      break;
    case AceButton::kEventLongReleased:
      Serial.println("Menu enc long-press released");
      break;
  }
}

void OLEDMenu::HandleButtonSelect(){
  if(auto page = m_page_stack.top()){
    // Let page handle select input first
    if(page->HandleSelect()){
      return;
    }
    
    if(auto selected_item = page->GetSelectedItem()){
      if(auto next_page = selected_item->GetTarget()){
        m_page_stack.push(next_page);
      }
    }
  }
}

void OLEDMenu::HandleButtonDoubleClick(){
  if(auto page = m_page_stack.top()){
    // Let page handle back input first
    if(page->HandleDoubleClick()){
      return;
    }
  }
  
  // Go back to previous page
  //m_page_stack.pop();
}

void OLEDMenu::HandleButtonBack(){
  // Let page handle back input first
  if(auto page = m_page_stack.top()){
    if(page->HandleBack()){
      return;
    }
  }
}

void OLEDMenu::draw(Adafruit_SH1107& display){
  if(auto page = m_page_stack.top())
    page->draw(display);
  else
    Serial.println("No menu page in stack");
}
