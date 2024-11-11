#ifdef ESP32_BLE_KEYBOARD_H
#pragma message("warning,not recommended to use with BleKeyboard Library 不建议和蓝牙键盘库一起使用")
#endif

// uncomment the following line to use NimBLE library
#define USE_NIMBLE
#ifndef ESP32_NKRO_BLE_KEYBOARD_H
#define ESP32_NKRO_BLE_KEYBOARD_H
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#if defined(USE_NIMBLE)

#include "NimBLECharacteristic.h"
#include "NimBLEHIDDevice.h"

#define BLEDevice NimBLEDevice
#define BLEServerCallbacks NimBLEServerCallbacks
#define BLECharacteristicCallbacks NimBLECharacteristicCallbacks
#define BLEHIDDevice NimBLEHIDDevice
#define BLECharacteristic NimBLECharacteristic
#define BLEAdvertising NimBLEAdvertising
#define BLEServer NimBLEServer

#else

#include "BLEHIDDevice.h"
#include "BLECharacteristic.h"

#endif // USE_NIMBLE

#include "Print.h"

#define NKRO_BLE_KEYBOARD_VERSION "0.0.1"
#define NKRO_BLE_KEYBOARD_VERSION_MAJOR 0
#define NKRO_BLE_KEYBOARD_VERSION_MINOR 0
#define NKRO_BLE_KEYBOARD_VERSION_REVISION 1

typedef uint8_t nkro_MediaKeyReport[2];

#ifndef ESP32_BLE_KEYBOARD_H

// 和官方USBHIDKeyBoard库一起使用时要注释掉关于按键的定义
//  const uint8_t KEY_LEFT_CTRL = 0x80;
//  const uint8_t KEY_LEFT_SHIFT = 0x81;
//  const uint8_t KEY_LEFT_ALT = 0x82;
//  const uint8_t KEY_LEFT_GUI = 0x83;
//  const uint8_t KEY_RIGHT_CTRL = 0x84;
//  const uint8_t KEY_RIGHT_SHIFT = 0x85;
//  const uint8_t KEY_RIGHT_ALT = 0x86;
//  const uint8_t KEY_RIGHT_GUI = 0x87;

// const uint8_t KEY_UP_ARROW = 0xDA;
// const uint8_t KEY_DOWN_ARROW = 0xD9;
// const uint8_t KEY_LEFT_ARROW = 0xD8;
// const uint8_t KEY_RIGHT_ARROW = 0xD7;
// const uint8_t KEY_BACKSPACE = 0xB2;
// const uint8_t KEY_TAB = 0xB3;
// const uint8_t KEY_RETURN = 0xB0;
// const uint8_t KEY_ESC = 0xB1;
// const uint8_t KEY_INSERT = 0xD1;
// const uint8_t KEY_PRTSC = 0xCE;
// const uint8_t KEY_DELETE = 0xD4;
// const uint8_t KEY_PAGE_UP = 0xD3;
// const uint8_t KEY_PAGE_DOWN = 0xD6;
// const uint8_t KEY_HOME = 0xD2;
// const uint8_t KEY_END = 0xD5;
// const uint8_t KEY_CAPS_LOCK = 0xC1;
// const uint8_t KEY_F1 = 0xC2;
// const uint8_t KEY_F2 = 0xC3;
// const uint8_t KEY_F3 = 0xC4;
// const uint8_t KEY_F4 = 0xC5;
// const uint8_t KEY_F5 = 0xC6;
// const uint8_t KEY_F6 = 0xC7;
// const uint8_t KEY_F7 = 0xC8;
// const uint8_t KEY_F8 = 0xC9;
// const uint8_t KEY_F9 = 0xCA;
// const uint8_t KEY_F10 = 0xCB;
// const uint8_t KEY_F11 = 0xCC;
// const uint8_t KEY_F12 = 0xCD;
// const uint8_t KEY_F13 = 0xF0;
// const uint8_t KEY_F14 = 0xF1;
// const uint8_t KEY_F15 = 0xF2;
// const uint8_t KEY_F16 = 0xF3;
// const uint8_t KEY_F17 = 0xF4;
// const uint8_t KEY_F18 = 0xF5;
// const uint8_t KEY_F19 = 0xF6;
// const uint8_t KEY_F20 = 0xF7;
// const uint8_t KEY_F21 = 0xF8;
// const uint8_t KEY_F22 = 0xF9;
// const uint8_t KEY_F23 = 0xFA;
// const uint8_t KEY_F24 = 0xFB;

// const uint8_t KEY_NUM_0 = 0xEA;
// const uint8_t KEY_NUM_1 = 0xE1;
// const uint8_t KEY_NUM_2 = 0xE2;
// const uint8_t KEY_NUM_3 = 0xE3;
// const uint8_t KEY_NUM_4 = 0xE4;
// const uint8_t KEY_NUM_5 = 0xE5;
// const uint8_t KEY_NUM_6 = 0xE6;
// const uint8_t KEY_NUM_7 = 0xE7;
// const uint8_t KEY_NUM_8 = 0xE8;
// const uint8_t KEY_NUM_9 = 0xE9;
// const uint8_t KEY_NUM_SLASH = 0xDC;
// const uint8_t KEY_NUM_ASTERISK = 0xDD;
// const uint8_t KEY_NUM_MINUS = 0xDE;
// const uint8_t KEY_NUM_PLUS = 0xDF;
// const uint8_t KEY_NUM_ENTER = 0xE0;
// const uint8_t KEY_NUM_PERIOD = 0xEB;

#define MediaKeyReport nkro_MediaKeyReport

const nkro_MediaKeyReport KEY_MEDIA_NEXT_TRACK = {1, 0};
const nkro_MediaKeyReport KEY_MEDIA_PREVIOUS_TRACK = {2, 0};
const nkro_MediaKeyReport KEY_MEDIA_STOP = {4, 0};
const nkro_MediaKeyReport KEY_MEDIA_PLAY_PAUSE = {8, 0};
const nkro_MediaKeyReport KEY_MEDIA_MUTE = {16, 0};
const nkro_MediaKeyReport KEY_MEDIA_VOLUME_UP = {32, 0};
const nkro_MediaKeyReport KEY_MEDIA_VOLUME_DOWN = {64, 0};
const nkro_MediaKeyReport KEY_MEDIA_WWW_HOME = {128, 0};
const nkro_MediaKeyReport KEY_MEDIA_LOCAL_MACHINE_BROWSER = {0, 1}; // Opens "My Computer" on Windows
const nkro_MediaKeyReport KEY_MEDIA_CALCULATOR = {0, 2};
const nkro_MediaKeyReport KEY_MEDIA_WWW_BOOKMARKS = {0, 4};
const nkro_MediaKeyReport KEY_MEDIA_WWW_SEARCH = {0, 8};
const nkro_MediaKeyReport KEY_MEDIA_WWW_STOP = {0, 16};
const nkro_MediaKeyReport KEY_MEDIA_WWW_BACK = {0, 32};
const nkro_MediaKeyReport KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION = {0, 64}; // Media Selection
const nkro_MediaKeyReport KEY_MEDIA_EMAIL_READER = {0, 128};

#endif

//  up to 136 keys and shift, ctrl etc at once
typedef struct
{
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[17];
} NkroBLEKeyReport;

typedef void (*ble_connect_event_cb_t)(bool isConnected);

class NkroBleKeyboard : public Print, public BLEServerCallbacks, public BLECharacteristicCallbacks
{
private:
  BLEHIDDevice *hid;
  BLECharacteristic *inputKeyboard;
  BLECharacteristic *outputKeyboard;
  BLECharacteristic *inputMediaKeys;
  BLEAdvertising *advertising;
  NkroBLEKeyReport _keyReport;

  std::string deviceName;
  std::string deviceManufacturer;
  uint8_t batteryLevel;
  bool connected = false;
  uint32_t _delay_ms = 7;
  void delay_ms(uint64_t ms);

  uint16_t vid = 0x05ac;
  uint16_t pid = 0x820a;
  uint16_t version = 0x0210;

  ble_connect_event_cb_t connect_cb = NULL;

public:
  nkro_MediaKeyReport _mediaKeyReport;
  NkroBleKeyboard(std::string deviceName = "ESP32-S3 BLE-KB", std::string deviceManufacturer = "Espressif", uint8_t batteryLevel = 100);
  void begin(void);
  void end(void);
  void sendReport(NkroBLEKeyReport *keys);
  void sendReport(nkro_MediaKeyReport *keys);
  size_t press(uint8_t k);
  size_t press(const nkro_MediaKeyReport k);
  size_t release(uint8_t k);
  size_t release(const nkro_MediaKeyReport k);
  size_t write(uint8_t c);
  size_t write(const nkro_MediaKeyReport c);
  size_t write(const uint8_t *buffer, size_t size);
  void releaseAll(void);
  bool isConnected(void);
  void setBatteryLevel(uint8_t level);
  void setName(std::string deviceName);
  void setDelay(uint32_t ms);

  void set_vendor_id(uint16_t vid);
  void set_product_id(uint16_t pid);
  void set_version(uint16_t version);

  void register_connect_event(ble_connect_event_cb_t eventCb);

protected:
  virtual void onStarted(BLEServer *pServer) {};
  virtual void onConnect(BLEServer *pServer) override;
  virtual void onDisconnect(BLEServer *pServer) override;
  virtual void onWrite(BLECharacteristic *me) override;
};

#endif // CONFIG_BT_ENABLED
#endif // ESP32_BLE_KEYBOARD_H
