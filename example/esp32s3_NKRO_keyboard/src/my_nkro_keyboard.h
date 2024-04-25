/*
  Keyboard.h

  Copyright (c) 2015, Arduino LLC
  Original code (pre-library): Copyright (c) 2011, Peter Barrett

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include "soc/soc_caps.h"
#if SOC_USB_OTG_SUPPORTED

#include "Print.h"
#include "USBHID.h"

#if CONFIG_TINYUSB_HID_ENABLED

#include "esp_event.h"

#define MY_REPORTID 0x63
#define MY_REPORT_SIZE 0x13
// #define USE_LASTEST_LIB 1 //最新的master分支esp32库增加了新的函数，旧版注释掉这行

ESP_EVENT_DECLARE_BASE(MY_NKRO_USB_KEYBOARD_EVENTS);

typedef enum
{
  MY_NKRO_USB_KEYBOARD_ANY_EVENT = ESP_EVENT_ANY_ID,
  MY_NKRO_USB_KEYBOARD_LED_EVENT = 0,
  MY_NKRO_USB_KEYBOARD_MAX_EVENT,
} my_nkro_usb_keyboard_event_t;

typedef union
{
  struct
  {
    uint8_t numlock : 1;
    uint8_t capslock : 1;
    uint8_t scrolllock : 1;
    uint8_t compose : 1;
    uint8_t kana : 1;
    uint8_t reserved : 3;
  };
  uint8_t leds;
} my_nkro_usb_keyboard_event_data_t;

#define KEY_NKRO_LEFT_CTRL 0x80
#define KEY_NKRO_LEFT_SHIFT 0x81
#define KEY_NKRO_LEFT_ALT 0x82
#define KEY_NKRO_LEFT_GUI 0x83
#define KEY_NKRO_RIGHT_CTRL 0x84
#define KEY_NKRO_RIGHT_SHIFT 0x85
#define KEY_NKRO_RIGHT_ALT 0x86
#define KEY_NKRO_RIGHT_GUI 0x87

#define KEY_NKRO_UP_ARROW 0xDA
#define KEY_NKRO_DOWN_ARROW 0xD9
#define KEY_NKRO_LEFT_ARROW 0xD8
#define KEY_NKRO_RIGHT_ARROW 0xD7
#define KEY_NKRO_MENU 0xFE
#define KEY_NKRO_SPACE 0x20
#define KEY_NKRO_BACKSPACE 0xB2
#define KEY_NKRO_TAB 0xB3
#define KEY_NKRO_RETURN 0xB0
#define KEY_NKRO_ESC 0xB1
#define KEY_NKRO_INSERT 0xD1
#define KEY_NKRO_DELETE 0xD4
#define KEY_NKRO_PAGE_UP 0xD3
#define KEY_NKRO_PAGE_DOWN 0xD6
#define KEY_NKRO_HOME 0xD2
#define KEY_NKRO_END 0xD5
#define KEY_NKRO_NUM_LOCK 0xDB
#define KEY_NKRO_CAPS_LOCK 0xC1
#define KEY_NKRO_F1 0xC2
#define KEY_NKRO_F2 0xC3
#define KEY_NKRO_F3 0xC4
#define KEY_NKRO_F4 0xC5
#define KEY_NKRO_F5 0xC6
#define KEY_NKRO_F6 0xC7
#define KEY_NKRO_F7 0xC8
#define KEY_NKRO_F8 0xC9
#define KEY_NKRO_F9 0xCA
#define KEY_NKRO_F10 0xCB
#define KEY_NKRO_F11 0xCC
#define KEY_NKRO_F12 0xCD
#define KEY_NKRO_F13 0xF0
#define KEY_NKRO_F14 0xF1
#define KEY_NKRO_F15 0xF2
#define KEY_NKRO_F16 0xF3
#define KEY_NKRO_F17 0xF4
#define KEY_NKRO_F18 0xF5
#define KEY_NKRO_F19 0xF6
#define KEY_NKRO_F20 0xF7
#define KEY_NKRO_F21 0xF8
#define KEY_NKRO_F22 0xF9
#define KEY_NKRO_F23 0xFA
#define KEY_NKRO_F24 0xFB
#define KEY_NKRO_PRINT_SCREEN 0xCE
#define KEY_NKRO_SCROLL_LOCK 0xCF
#define KEY_NKRO_PAUSE 0xD0

#define LED_NKRO_NUMLOCK 0x01
#define LED_NKRO_CAPSLOCK 0x02
#define LED_NKRO_SCROLLLOCK 0x04
#define LED_NKRO_COMPOSE 0x08
#define LED_NKRO_KANA 0x10


//  up to 136 keys and shift, ctrl etc at once
typedef struct
{
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[17];
} NkroKeyReport;

typedef struct
{
  uint8_t modifier;
  uint8_t reserved;
  uint8_t keycode[17];
} NkroKeyReport_t;

class MY_NKRO_USBKeyboard : public USBHIDDevice, public Print
{
private:
  USBHID hid;
  NkroKeyReport _keyReport;
  my_nkro_usb_keyboard_event_data_t _keyboard_led_state;
  bool shiftKeyReports;

public:
  MY_NKRO_USBKeyboard(void);
  void begin(void);
  void end(void);
  size_t write(uint8_t k);
  size_t write(const uint8_t *buffer, size_t size);
  size_t press(uint8_t k);
  size_t release(uint8_t k);
  void releaseAll(void);
  void sendReport(NkroKeyReport *keys);
  void setShiftKeyReports(bool set);
  my_nkro_usb_keyboard_event_data_t get_keyboard_led_state();
  bool hid_device_ready();

  // raw functions work with TinyUSB's HID_KEY_* macros
  size_t pressRaw(uint8_t k);
  size_t releaseRaw(uint8_t k);

  void onEvent(esp_event_handler_t callback);
  void onEvent(my_nkro_usb_keyboard_event_t event, esp_event_handler_t callback);

  // internal use
  uint16_t _onGetDescriptor(uint8_t *buffer);
  void _onOutput(uint8_t report_id, const uint8_t *buffer, uint16_t len);
};

#endif /* CONFIG_TINYUSB_HID_ENABLED */
#endif /* SOC_USB_OTG_SUPPORTED */

// const uint8_t my_nkro_kb_descriptor[] = {
//     0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
//     0x09, 0x06, // Usage (Keyboard)
//     0xA1, 0x01, // Collection (Application)

//     0x85, MY_REPORTID, //   Report ID (99)
//     0x05, 0x07, //   Usage Page (Kbrd/Keypad)
//     0x15, 0x00, //   Logical Minimum (0)
//     0x25, 0x01, //   Logical Maximum (1)

//     // 修饰键 1B
//     0x19, 0xE0, //   Usage Minimum (0xE0)
//     0x29, 0xE7, //   Usage Maximum (0xE7)
//     0x75, 0x01, //   Report Size (1)
//     0x95, 0x08, //   Report Count (8)
//     0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

//     // 保留 1B
//     0x75, 0x01, //   Report Size (1)
//     0x95, 0x08, //   Report Count (8)
//     0x81, 0x03, //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

//     // led状态（主机到设备） 1B
//     0x05, 0x08, //   Usage Page (LEDs)
//     0x19, 0x01, //   Usage Minimum (Num Lock)
//     0x29, 0x08, //   Usage Maximum (Do Not Disturb)
//     0x75, 0x01, //   Report Size (1)
//     0x95, 0x08, //   Report Count (8)
//     0x91, 0x02, //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

//     // 按键 17B
//     0x05, 0x07, //   Usage Page (Kbrd/Keypad)
//     0x19, 0x00, //   Usage Minimum (0x00)
//     0x29, 0x87, //   Usage Maximum (0x87)
//     0x75, 0x01, //   Report Size (1)
//     0x95, 0x88, //   Report Count (136)
//     0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

//     0xC0, // End Collection
//     // 54 bytes
// };