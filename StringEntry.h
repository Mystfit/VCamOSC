#pragma once

#include <string>
#include "Item.h"

#define CURSOR_BLINK_SPEED 5
#define MASK 0xFF
#define MASK_C "\xFF"
#define DEFAULT_VALID_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!\"#$%&\'()*+,-./:;<=>?@[\\]^_ `{|}~"

enum EntryMode {
  HOVER,
  SELECT_CHAR,
  OVERWRITE,
  ENTRYMODE_SIZE = 3
};

class StringEntry : public Item {
public:
  StringEntry(const char* name, const char* display);
  StringEntry(const char* name, std::shared_ptr<std::string> target_string, const char* valid_chars = DEFAULT_VALID_CHARS, const std::string& mask = "");
  
  bool ScrollForwards() override;
  bool ScrollBackwards() override;
  
  bool HandleSelect() override;
  bool HandleBack() override;

  bool IsEditable();
  void SetDisplayText(const char* display);

  void draw(Adafruit_SH1107& display) override;
  
  
private:
  // String entry actions 
  void ScrollCursor(bool reverse = false);
  void ScrollCharToEnter(bool reverse = false);
  void DoOverwrite();
  void DoBackspace();

  std::string MaskString(const std::string& input, const std::string& mask);
  void CommitBuffer();
  
  const std::string m_valid_chars;
  std::string m_mask;
  int m_cursor_idx;
  int m_char_idx;
  int m_max_viewable_chars;
  int m_viewable_chars_offset;
  bool m_cursor_visible;
  int m_cursor_blink_counter;
  std::string m_buffer;
  std::shared_ptr<std::string> m_target_string;
  EntryMode m_entry_mode;
};