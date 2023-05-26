#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoOSCWiFi.h>
#include <ESP32Encoder.h>
#include <AceButton.h>
#include <functional>
#include <iomanip>
#include <sstream>
#include "esp_wpa2.h"
#include "OLEDMenu.h"
#include "Page.h"
#include "Item.h"
#include "StringEntry.h"
#include "ToggleEntry.h"

using namespace ace_button;

#define VCAM_PREF_NS "VCamOSC"
#define EEPROM_SETTINGS_VALID 0xDEADBEEF
#define NEOPIXEL_I2C_POWER 2
#define ZOOM_PIN 36
#define FOCUS_PIN 37
#define SAMPLES_ENC_PIN_1 33
#define SAMPLES_ENC_PIN_2 32
#define MENU_ENC_PIN 39
#define MAX_SMOOTH_SAMPLES 512
#define IP_MASK MASK_C MASK_C MASK_C "." MASK_C MASK_C MASK_C "." MASK_C MASK_C MASK_C "." MASK_C MASK_C MASK_C

// WiFi
std::shared_ptr<std::string> ssid = std::make_shared<std::string>();      
std::shared_ptr<std::string> password = std::make_shared<std::string>();  

// Wifi authentication
std::shared_ptr<std::string> EAP_identity = std::make_shared<std::string>();      
std::shared_ptr<std::string> EAP_password = std::make_shared<std::string>();
// bool useEAP = false;

// OSC destination
std::shared_ptr<std::string> serverAddr = std::make_shared<std::string>();
int serverPort = 1234;

// OSC paths
std::string zoomPath = "/vcam/zoom";
std::string focusPath = "/vcam/focus";

// Smoothing
float zoomVals[MAX_SMOOTH_SAMPLES];
float focusVals[MAX_SMOOTH_SAMPLES];
float smoothedZoomVal = 0.0f;
float smoothedFocusVal = 0.0f;
size_t numSmoothingSamples = 10;
size_t sample_idx = 0;

// Controls
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
ESP32Encoder MenuEnc;
int lastEncoderVal;

AceButton MenuEncBtn;

// UI objects
OLEDMenu Menu = OLEDMenu();
std::shared_ptr<ToggleEntry> UseEAPEntry = std::make_shared<ToggleEntry>("Use EAP", std::vector<std::string>{"Disabled", "Enabled"});
std::shared_ptr<StringEntry> LocalIPText;
std::shared_ptr<StringEntry> FocusText;
std::shared_ptr<StringEntry> ZoomText;

// Forwards
void handleEvent(AceButton*, uint8_t, uint8_t);

void enableInternalPower() {
  pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_I2C_POWER, HIGH);
  Serial.print("Setting neopixel I2C power to HIGH");
}

void disableInternalPower() {
  pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_I2C_POWER, LOW);
}

void ReadSettings(){
  Preferences preferences;
  preferences.begin(VCAM_PREF_NS, false); 
  bool valid_prefs = preferences.getULong("valid_prefs", 0) == EEPROM_SETTINGS_VALID;

  String ssid_s = preferences.getString("ssid", "");
  ssid->clear();
  ssid->resize(ssid_s.length());
  ssid->replace(size_t(0), ssid_s.length(), ssid_s.c_str());

  String password_s = preferences.getString("password", "");
  password->clear();
  password->resize(password_s.length());
  password->replace(size_t(0), password_s.length(), password_s.c_str());

  UseEAPEntry->SetSelectedIndex((size_t)preferences.getBool("use_eap", false));

  String eap_identity_s = preferences.getString("eap_identity", "");
  EAP_identity->clear();
  EAP_identity->resize(eap_identity_s.length());
  EAP_identity->replace(size_t(0), eap_identity_s.length(), eap_identity_s.c_str());

  String eap_password_s = preferences.getString("eap_password", "");
  EAP_password->clear();
  EAP_password->resize(eap_password_s.length());
  EAP_password->replace(size_t(0), eap_password_s.length(), eap_password_s.c_str());

  Serial.println("About to read target addr");
  IPAddress target_addr(preferences.getULong("target_addr", 0xFFFFFFFF));
  //String target_addr_str = target_addr.toString();

  std::stringstream addr_padded[4];
  for(size_t i = 0; i < 4; ++i){
    addr_padded[i] << std::setfill('0') << std::setw(3) << target_addr[i]; 
  }
  std::stringstream addr_padded_combined;
  addr_padded_combined << addr_padded[0].str() << "." << addr_padded[1].str() << "." << addr_padded[2].str() << "." << addr_padded[3].str();

  serverAddr->clear();
  serverAddr->resize(addr_padded_combined.str().length());
  serverAddr->replace(size_t(0), addr_padded_combined.str().length(), addr_padded_combined.str().c_str());

  preferences.end();
}

void WriteSettings(){
  Preferences preferences;
  preferences.begin(VCAM_PREF_NS, false); 
  preferences.putULong("valid_prefs", EEPROM_SETTINGS_VALID);
  preferences.putString("ssid", ssid->c_str());
  preferences.putString("password", password->c_str());
  preferences.putBool("use_eap", UseEAPEntry->GetSelectedIndex());
  preferences.putString("eap_identity", EAP_identity->c_str());
  preferences.putString("eap_password", EAP_password->c_str());

  IPAddress target_addr;
  target_addr.fromString(serverAddr->c_str());
  preferences.putULong("target_addr", (uint32_t)target_addr);
  preferences.end();
}

IPAddress FormatPaddedIP(String address){
  uint8_t octets[4];
  for(size_t idx = 0; idx < 4; ++idx){
    octets[idx] = static_cast<uint8_t>(std::atoi(address.substring(idx * 4, idx * 4 + 3).c_str()));
  }
 
  return IPAddress(octets);
}

void ConnectToWifi(){
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);

  if (UseEAPEntry->GetSelectedIndex()){
    Serial.print("Connecting with ssid ");
    Serial.print(ssid->c_str());
    Serial.print(" and EAP identity ");
    Serial.println(EAP_identity->c_str());
    WiFi.begin(ssid->c_str(), WPA2_AUTH_PEAP, EAP_identity->c_str(), EAP_identity->c_str(), EAP_password->c_str());
  }else{
    Serial.print("Connecting with ssid ");
    Serial.print(ssid->c_str());
    Serial.print(" and password ");
    Serial.println(password->c_str());
    WiFi.begin(ssid->c_str(), password->c_str());
  }

  // Wifi connection progress
  LocalIPText->SetDisplayText("Connecting...");
  
  size_t connection_count = 10;
  while (WiFi.status() != WL_CONNECTED && connection_count > 0) {
    delay(500);
    Serial.print(".");
    connection_count--;
  }

  if(WiFi.status() != WL_CONNECTED){
    // Could not connect
    LocalIPText->SetDisplayText("Connection failed");
  } else {
    // Connected 
    // Set up OSC publishers
    IPAddress destinationAddr(FormatPaddedIP(String(serverAddr->c_str())));
    String zoomPathStr(zoomPath.c_str());
    String focusPathStr(focusPath.c_str());
    Serial.print("Starting OSC publish for address ");
    Serial.print(zoomPathStr);
    Serial.print(" to destination ");
    Serial.print(destinationAddr.toString());
    Serial.print(":");
    Serial.println(serverPort);
    OscWiFi.publish(destinationAddr.toString(), serverPort, zoomPathStr, smoothedZoomVal)->setFrameRate(60.f);
    OscWiFi.publish(destinationAddr.toString(), serverPort, focusPathStr, smoothedFocusVal)->setFrameRate(60.f);

    // Display local IP in UI
    LocalIPText->SetDisplayText(WiFi.localIP().toString().c_str());
    
    // Update settings
    WriteSettings();
  }
}

std::shared_ptr<Page> CreateWifiMenuPage(){
  std::shared_ptr<Page> wifi_pg = std::make_shared<Page>("WiFi Settings");

  wifi_pg->AddItem(std::make_shared<StringEntry>("SSID", ssid));
  wifi_pg->AddItem(std::make_shared<StringEntry>("Password", password));
  wifi_pg->AddItem(UseEAPEntry);
  wifi_pg->AddItem(std::make_shared<StringEntry>("EAP identity", EAP_identity));
  wifi_pg->AddItem(std::make_shared<StringEntry>("EAP password", EAP_password));
  wifi_pg->AddItem(std::make_shared<Item>("Save", nullptr, true, WriteSettings));
  wifi_pg->AddItem(std::make_shared<Item>("Back", nullptr, true, std::bind(&OLEDMenu::PopPage, &Menu)));
  return wifi_pg;
}

std::shared_ptr<Page> CreateControlsMenuPage(){
  std::shared_ptr<Page> controls_pg = std::make_shared<Page>("Controls");
  controls_pg->AddItem(std::make_shared<Item>("Smoothing", nullptr, false));
  
  ZoomText = std::make_shared<StringEntry>(zoomPath.c_str(), "");
  controls_pg->AddItem(ZoomText);
  FocusText = std::make_shared<StringEntry>(focusPath.c_str(), "");
  controls_pg->AddItem(FocusText);

  controls_pg->AddItem(std::make_shared<Item>("Back", nullptr, true, std::bind(&OLEDMenu::PopPage, &Menu)));

  return controls_pg;
}

std::shared_ptr<Page> CreateConnectionPage(){
  std::shared_ptr<Page> connection_pg = std::make_shared<Page>("WiFi Connection");
  connection_pg->AddItem(std::make_shared<Item>("Connect", nullptr, true, ConnectToWifi));
  
  LocalIPText = std::make_shared<StringEntry>("Local IP", "");
  connection_pg->AddItem(LocalIPText);
  connection_pg->AddItem(std::make_shared<StringEntry>("Target", serverAddr, "0123456789", IP_MASK));
  connection_pg->AddItem(std::make_shared<Item>("Back", nullptr, true, std::bind(&OLEDMenu::PopPage, &Menu)));

  return connection_pg;
}

std::shared_ptr<Page> CreateMainMenu(){
  auto main_menu_pg = std::make_shared<Page>("Main menu");
  auto wifi_pg = CreateWifiMenuPage();
  auto connection_pg = CreateConnectionPage();
  auto controls_pg = CreateControlsMenuPage();
  
  main_menu_pg->AddItem(std::make_shared<Item>(wifi_pg->GetName(),  wifi_pg));
  main_menu_pg->AddItem(std::make_shared<Item>(connection_pg->GetName(), connection_pg));
  main_menu_pg->AddItem(std::make_shared<Item>(controls_pg->GetName(), controls_pg));

  return main_menu_pg;
}

void setup() {
  // Need to enable the STEMMA connector on ESP32
  enableInternalPower();

  Serial.begin(9600);
  
  // Read settings from EEPROM
  ReadSettings();

  // Setup encoder
  ESP32Encoder::useInternalWeakPullResistors = UP;
  MenuEnc.attachSingleEdge(SAMPLES_ENC_PIN_1, SAMPLES_ENC_PIN_2);
  MenuEnc.setCount(0);
  
  // Set initial smoothing values for analog inputs
  for (int idx = 0; idx < numSmoothingSamples; ++idx) {
    focusVals[idx] = analogRead(FOCUS_PIN);
    zoomVals[idx] = analogRead(ZOOM_PIN);
  }
  
  // Setup menu button
  pinMode(MENU_ENC_PIN, INPUT);
  MenuEncBtn.init(MENU_ENC_PIN, LOW);
  ButtonConfig* buttonConfig = MenuEncBtn.getButtonConfig();
  buttonConfig->setEventHandler(handleEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressClickBeforeDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);

  Serial.println("VCam OSC starting up");
  delay(250); // wait for the OLED to power up
  display.begin(0x3C, true); // Address 0x3C default

  Serial.println("OLED begun");

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(1000);

  // Clear the buffer.
  display.clearDisplay();
  display.display();
  display.setRotation(1);
  
  // Set Text defaults
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);

  auto main_page = CreateMainMenu();
  Menu.AddPage(main_page);
  Menu.PushPage(main_page);
  
  //ConnectToWifi()
}

void read_zoom_focus_dials(){
  numSmoothingSamples = min(MAX_SMOOTH_SAMPLES, max(int(numSmoothingSamples), 1));

  focusVals[sample_idx] = analogRead(FOCUS_PIN) / 4096.0f;
  zoomVals[sample_idx] = analogRead(ZOOM_PIN) / 4096.0f;
  sample_idx = (sample_idx + 1) % numSmoothingSamples;

  float smoothed_zoom = 0.0f;
  float smoothed_focus = 0.0f;
  for (size_t idx = 0; idx < numSmoothingSamples; ++idx) {
    smoothed_zoom += focusVals[idx];
    smoothed_focus += zoomVals[idx];
  }
  smoothedZoomVal = smoothed_zoom /= float(numSmoothingSamples);
  smoothedFocusVal = smoothed_focus /= float(numSmoothingSamples);
}

void loop() {
  MenuEncBtn.check();
  
  // Legacy way of reading zoom/focus vals
  read_zoom_focus_dials();
  
  int encoderDelta = MenuEnc.getCount() - lastEncoderVal;
  lastEncoderVal = MenuEnc.getCount();
  
  if(encoderDelta > 0){
    Menu.ScrollForwards();
  } else if(encoderDelta < 0){
    Menu.ScrollBackwards();
  }

  // Visualize values
  char zoom_buf[6] = {'\0'};
  sprintf(zoom_buf, "%.3f", smoothedZoomVal);
  ZoomText->SetDisplayText(&zoom_buf[0]);
  
  char focus_buf[6] = {'\0'};
  sprintf(focus_buf, "%.3f", smoothedFocusVal);
  FocusText->SetDisplayText(&focus_buf[0]);

  // Update OLED gui
  Menu.draw(display);

  // Update OSC publishers
  OscWiFi.post();
}

// The event handler for the button.
void handleEvent(AceButton* /* button */, uint8_t eventType, uint8_t /* buttonState */) {
  Menu.HandleButton(eventType);
}