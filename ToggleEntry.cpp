#include "ToggleEntry.h"

ToggleEntry::ToggleEntry(const char* name, std::vector<std::string> values) : 
  m_values(values),
  Item(name, nullptr, true)
{}

bool ToggleEntry::HandleSelect(){
  m_selected_idx = (m_selected_idx + 1) % m_values.size();
  return true;
}

void ToggleEntry::draw(Adafruit_SH1107& display){
  display.setTextColor(SH110X_WHITE, SH110X_BLACK);
  
  display.print(this->GetName());
  display.print(": ");

  if(IsSelectable()){
    if(this->IsSelected()){
        display.setTextColor(SH110X_BLACK, SH110X_WHITE);
        display.print(">");
    } else {
        display.setTextColor(SH110X_WHITE, SH110X_BLACK);
        display.print(" ");
    }
  }
  display.println(GetSelectedValue());
}

const char* ToggleEntry::GetSelectedValue(){
  if(m_selected_idx < m_values.size()){
    return m_values[m_selected_idx].c_str();
  }
  return "";
}

size_t ToggleEntry::GetSelectedIndex(){
  return m_selected_idx;
}

void ToggleEntry::SetSelectedIndex(size_t idx){
  m_selected_idx = constrain(idx, 0, m_values.size());
}