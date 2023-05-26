#pragma once
#include <memory>
#include <string>
#include <functional>
#include <Adafruit_SH110X.h>


// Forwards
class Page;
class Item;

// Helper type for callable items
typedef std::function<void()> CallEvent;

class Item {
public:
  Item(const char* name, std::shared_ptr<Page> page, bool selectable = true, CallEvent called_cb = nullptr);
  
  void SetSelected(bool state);
  bool IsSelected();
  bool IsSelectable();
  const char* GetName();
  std::shared_ptr<Page> GetTarget();
  
  virtual bool ScrollForwards();
  virtual bool ScrollBackwards();
  virtual bool HandleSelect();
  virtual bool HandleBack();
  
  void operator()();
  
  virtual void draw(Adafruit_SH1107& display);

private:
  bool m_selected;
  bool m_selectable;
  std::string m_name;
  std::shared_ptr<Page> m_target;
  CallEvent m_called_cb;
};
