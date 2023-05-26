#pragma once

#include <vector>
#include "Item.h"

class ToggleEntry : public Item {
public:
  ToggleEntry(const char* name, std::vector<std::string> values);

  bool HandleSelect() override;

  void draw(Adafruit_SH1107& display) override;

  const char* GetSelectedValue();
  
  size_t GetSelectedIndex();
  void SetSelectedIndex(size_t idx);

private:
  std::vector<std::string> m_values;
  size_t m_selected_idx = 0;
};