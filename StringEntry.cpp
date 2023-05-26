#include "StringEntry.h"
#include <Adafruit_GFX.h>

StringEntry::StringEntry(const char* name, const char* display) : 
  Item(name, nullptr, false),
  m_valid_chars(""),
  m_mask(""),
  m_cursor_idx(0),
  m_char_idx(0),
  m_target_string(nullptr),
  m_buffer(display),
  m_entry_mode(EntryMode::HOVER),
  m_cursor_visible(true),
  m_cursor_blink_counter(0)
{
  // Max viewable chars = (Screen pixel width / character width) - name offset
  m_viewable_chars_offset = strlen(GetName()) + 2;
  m_max_viewable_chars = (128 / 6) - m_viewable_chars_offset;
}

StringEntry::StringEntry(const char* name, std::shared_ptr<std::string> target_string, const char* valid_chars, const std::string& mask) :
  Item(name, nullptr, true),
  m_valid_chars(valid_chars),
  m_mask(mask),
  m_cursor_idx(0),
  m_char_idx(0),
  m_target_string(target_string),
  m_buffer(*target_string.get()),
  m_entry_mode(EntryMode::HOVER),
  m_cursor_visible(true),
  m_cursor_blink_counter(0)
{
  // Max viewable chars = (Screen pixel width / character width) - name offset
  m_viewable_chars_offset = strlen(GetName()) + 2;
  m_max_viewable_chars = (128 / 6) - m_viewable_chars_offset;
}

bool StringEntry::ScrollForwards(){
  if(m_entry_mode == EntryMode::HOVER){
    return false;
  }
  
  if(m_entry_mode == EntryMode::SELECT_CHAR){
    ScrollCursor();
    return true;
  }
  
  if(m_entry_mode == EntryMode::OVERWRITE){
    ScrollCharToEnter();
    return true;
  }

  return false;
}

bool StringEntry::ScrollBackwards(){
  if(m_entry_mode == EntryMode::HOVER){
    return false;
  }
  
  if(m_entry_mode == EntryMode::SELECT_CHAR){
    ScrollCursor(true);
    return true;
  }
  
  if(m_entry_mode == EntryMode::OVERWRITE){
    ScrollCharToEnter(true);
    return true;
  }

  return false;
}

bool StringEntry::HandleSelect(){
  switch(m_entry_mode){
    case EntryMode::HOVER:
      break;
    case EntryMode::SELECT_CHAR:
      DoBackspace();
      break;
    case EntryMode::OVERWRITE:
      DoOverwrite();
      break;
  }
  
  return true;
}

bool StringEntry::HandleBack(){
  if(!IsEditable())
    return false;
  
  EntryMode next_mode = EntryMode(((int)m_entry_mode + 1) % (int)EntryMode::ENTRYMODE_SIZE);
  
  // If buffer is empty, default to overwrite mode to add characters
  if(m_entry_mode == EntryMode::HOVER && !m_buffer.size()){
    next_mode = EntryMode::OVERWRITE;
  }
  m_entry_mode = next_mode;

  
  if(m_entry_mode == EntryMode::OVERWRITE){
    // Set picked character to the character underneath the cursor
    m_char_idx = (m_cursor_idx < m_buffer.size()) ? m_valid_chars.find_first_of(m_buffer[m_cursor_idx]) : 0;
  } else if(m_entry_mode == EntryMode::SELECT_CHAR){
    m_cursor_idx = std::max(std::min(m_cursor_idx, (int)m_buffer.size()-1), 0);
    m_cursor_visible = true;
    m_cursor_blink_counter = 0;
  }
  return true;
}

bool StringEntry::IsEditable(){
  return m_target_string != nullptr;
}

void StringEntry::SetDisplayText(const char* display){
  m_buffer = display;
}

void StringEntry::draw(Adafruit_SH1107& display){
  display.setTextColor(SH110X_WHITE, SH110X_BLACK);
  
  if(IsSelected()){
    switch(m_entry_mode){
      case EntryMode::HOVER:
        display.print(">");
        break;
      case EntryMode::SELECT_CHAR:
        display.print("_");
        break;
      case EntryMode::OVERWRITE:
        display.print("+");
        break;
    }
  } else {
    display.print(" ");
  }
  display.print(GetName());
  display.print(":");

  // Mask the display string
  std::string masked_buffer = MaskString(m_buffer, m_mask);
  
  // In hover mode, just draw the whole buffer
  if(m_entry_mode == EntryMode::HOVER){
    if(IsSelected() && IsEditable()){
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    }
    display.println(masked_buffer.c_str());
  }
  
  // Char selection and entry need to split the buffer and draw a cursor
  if(m_entry_mode == EntryMode::SELECT_CHAR || m_entry_mode == EntryMode::OVERWRITE){
    // Update carat blink
    if(++m_cursor_blink_counter % CURSOR_BLINK_SPEED == 0){
      m_cursor_visible = !m_cursor_visible;
      m_cursor_blink_counter = 0;
    }

    // Allow cursor to scroll offscreen if beyond the viewable area
    int cursor_offscreen_trim = 0;
    if(m_cursor_idx >= m_max_viewable_chars){
      cursor_offscreen_trim = m_cursor_idx - m_max_viewable_chars + 1;
    }

    // Split text before and after the cursor
    std::string buffer_firsthalf;
    std::string buffer_secondhalf;
    if(masked_buffer.size()){
      buffer_firsthalf = masked_buffer.substr(cursor_offscreen_trim, m_cursor_idx-cursor_offscreen_trim);  
      buffer_secondhalf = masked_buffer.substr(std::min(m_cursor_idx + 1, (int)masked_buffer.size()-1), (int)masked_buffer.size() - m_cursor_idx);  
    }
   
    // Show first half of buffer up to the cursor
    display.setTextColor(SH110X_WHITE, SH110X_BLACK);
    display.print(buffer_firsthalf.c_str());
    
    // Blink the cursor
    if(m_cursor_visible){
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    } else {
      display.setTextColor(SH110X_WHITE, SH110X_BLACK);
    }

    // Draw character at cursor
    char cursor_char;
    if(m_entry_mode == EntryMode::OVERWRITE){
      // Show current selected character to insert
      cursor_char = m_valid_chars[std::min(std::max(0, m_char_idx), (int)m_valid_chars.size()-1)];
    } else if(m_entry_mode == EntryMode::SELECT_CHAR) {
      // Highlight selected character
      if(masked_buffer.size())
        cursor_char = m_buffer[std::max(std::min(m_cursor_idx, (int)masked_buffer.size()-1), 0)];
      else
        cursor_char = ' ';
    }
    display.print(cursor_char);

    // Show second half of buffer after the cursor
    if(m_cursor_idx < masked_buffer.size()-1){
      display.setTextColor(SH110X_WHITE, SH110X_BLACK);
      display.print(buffer_secondhalf.c_str());
    }
    display.println("");
  }
}

void StringEntry::DoOverwrite(){
  if(m_char_idx >= 0){
    char append_char = m_valid_chars[m_char_idx];
    if(m_cursor_idx >= m_buffer.size()){
      m_buffer += append_char;
    } else {
      m_buffer[m_cursor_idx] = append_char; 
    }
    
    // Move cursor forwards
    ScrollCursor(false);

    // If there is a character under the next scrolled position, use that as our starting char
    m_char_idx = (m_cursor_idx < m_buffer.size()) ? m_valid_chars.find_first_of(m_buffer[m_cursor_idx]) : 0;
    
    // Save buffered changes to the string we're modifying
    CommitBuffer();
  }
}

void StringEntry::DoBackspace(){
  if(m_buffer.size()){
    m_buffer.erase(m_cursor_idx, 1);
    m_cursor_idx--;
    CommitBuffer();
  }
}

void StringEntry::CommitBuffer(){
  if(m_target_string){
      std::string masked_buffer = MaskString(m_buffer, m_mask);
      m_target_string->clear();
      m_target_string->replace(size_t(0), masked_buffer.size(), masked_buffer);
    }
}

std::string StringEntry::MaskString(const std::string& input, const std::string& mask){
  if(!input.size() || !mask.size()){
    return input;
  }

  std::string output;
  for(size_t idx = 0; idx < std::min(mask.size(), input.size()); ++idx){
    output += (mask[idx] == MASK) ? input[idx] : mask[idx];
  }
  return output;
}


void StringEntry::ScrollCursor(bool reverse){
  int cursor_idx = m_cursor_idx + (reverse ? -1 : 1);

  // Skip over characters not in the mask
  if(m_mask.size()){
    while(cursor_idx < (int)m_mask.size() && cursor_idx >= 0){
      // If we have a mask, make sure the cursor stays within it and skips mask characters
      if(m_mask[cursor_idx] == MASK){
        m_cursor_idx = cursor_idx;
        break;
      }
      cursor_idx = cursor_idx + (reverse ? -1 : 1);
    }
  } else {
    int overwrite_last_char_idx = (m_entry_mode == EntryMode::OVERWRITE) ? (int)m_buffer.size() : (int)m_buffer.size() - 1;
    m_cursor_idx = std::max(std::min(cursor_idx, overwrite_last_char_idx), 0);
  }
  
  m_cursor_visible = true;
  m_cursor_blink_counter = 0;
}

void StringEntry::ScrollCharToEnter(bool reverse){
  if(reverse){
    m_char_idx = (m_char_idx - 1) < 0 ? m_valid_chars.size() - 1 : m_char_idx-1;
  } else {
    m_char_idx = (m_char_idx + 1) % m_valid_chars.size();
  }
}