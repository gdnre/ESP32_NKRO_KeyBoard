/*
  Keyboard.cpp

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
#include "USBHID.h"
#if SOC_USB_OTG_SUPPORTED

#if CONFIG_TINYUSB_HID_ENABLED

#include "my_nkro_keyboard.h"

ESP_EVENT_DEFINE_BASE(MY_NKRO_USB_KEYBOARD_EVENTS);
esp_err_t arduino_usb_event_post(esp_event_base_t event_base, int32_t event_id, void *event_data, size_t event_data_size, TickType_t ticks_to_wait);
esp_err_t arduino_usb_event_handler_register_with(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void *event_handler_arg);

static const uint8_t report_descriptor[] = {
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06, // Usage (Keyboard)
    0xA1, 0x01, // Collection (Application)

    0x85, MY_REPORTID, //   Report ID (99)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)

    // 修饰键 1B
    0x19, 0xE0, //   Usage Minimum (0xE0)
    0x29, 0xE7, //   Usage Maximum (0xE7)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x08, //   Report Count (8)
    0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

    // 保留 1B
    0x75, 0x01, //   Report Size (1)
    0x95, 0x08, //   Report Count (8)
    0x81, 0x03, //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

    // led状态（主机到设备） 1B
    0x05, 0x08, //   Usage Page (LEDs)
    0x19, 0x01, //   Usage Minimum (Num Lock)
    0x29, 0x08, //   Usage Maximum (Do Not Disturb)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x08, //   Report Count (8)
    0x91, 0x02, //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

    // 按键 17B
    0x05, 0x07, //   Usage Page (Kbrd/Keypad)
    0x19, 0x00, //   Usage Minimum (0x00)
    0x29, 0x87, //   Usage Maximum (0x87)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x88, //   Report Count (136)
    0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0xC0, // End Collection
    // 54 bytes
};

#ifdef USE_LASTEST_LIB

MY_NKRO_USBKeyboard::MY_NKRO_USBKeyboard() : hid(HID_ITF_PROTOCOL_KEYBOARD), shiftKeyReports(true)
{

    static bool initialized = false;
    if (!initialized)
    {
        _keyboard_led_state.leds = 0;
        initialized = true;
        memset(&_keyReport, 0, sizeof(NkroKeyReport));
        hid.addDevice(this, sizeof(report_descriptor));
    }
}
#else
MY_NKRO_USBKeyboard::MY_NKRO_USBKeyboard() : hid(), shiftKeyReports(true)
{
    static bool initialized = false;
    if (!initialized)
    {
        _keyboard_led_state.leds = 0;
        initialized = true;
        memset(&_keyReport, 0, sizeof(NkroKeyReport));
        hid.addDevice(this, sizeof(report_descriptor));
    }
}
#endif

uint16_t MY_NKRO_USBKeyboard::_onGetDescriptor(uint8_t *dst)
{
    memcpy(dst, report_descriptor, sizeof(report_descriptor));
    return sizeof(report_descriptor);
}

void MY_NKRO_USBKeyboard::begin()
{
    hid.begin();
}

void MY_NKRO_USBKeyboard::end() {}

void MY_NKRO_USBKeyboard::onEvent(esp_event_handler_t callback)
{
    onEvent(MY_NKRO_USB_KEYBOARD_ANY_EVENT, callback);
}
void MY_NKRO_USBKeyboard::onEvent(my_nkro_usb_keyboard_event_t event, esp_event_handler_t callback)
{
    arduino_usb_event_handler_register_with(MY_NKRO_USB_KEYBOARD_EVENTS, event, callback, this);
}

void MY_NKRO_USBKeyboard::_onOutput(uint8_t report_id, const uint8_t *buffer, uint16_t len)
{
    if (report_id == MY_REPORTID)
    { // id改成自己的id
        // my_nkro_usb_keyboard_event_data_t p;
        _keyboard_led_state.leds = buffer[0];
        arduino_usb_event_post(
            MY_NKRO_USB_KEYBOARD_EVENTS, MY_NKRO_USB_KEYBOARD_LED_EVENT, &_keyboard_led_state, sizeof(my_nkro_usb_keyboard_event_data_t), portMAX_DELAY);
    }
}

void MY_NKRO_USBKeyboard::sendReport(NkroKeyReport *keys)
{
    NkroKeyReport_t report;
    report.reserved = 0;
    report.modifier = keys->modifiers;
    memcpy(report.keycode, keys->keys, 17); // 修改成自己的大小
    hid.SendReport(MY_REPORTID, &report, sizeof(report));
}

void MY_NKRO_USBKeyboard::setShiftKeyReports(bool set)
{
    shiftKeyReports = set;
}

#define SHIFT 0x80
const uint8_t _asciimap[128] = {
    0x00, // NUL
    0x00, // SOH
    0x00, // STX
    0x00, // ETX
    0x00, // EOT
    0x00, // ENQ
    0x00, // ACK
    0x00, // BEL
    0x2a, // BS   Backspace
    0x2b, // TAB  Tab
    0x28, // LF   Enter
    0x00, // VT
    0x00, // FF
    0x00, // CR
    0x00, // SO
    0x00, // SI
    0x00, // DEL
    0x00, // DC1
    0x00, // DC2
    0x00, // DC3
    0x00, // DC4
    0x00, // NAK
    0x00, // SYN
    0x00, // ETB
    0x00, // CAN
    0x00, // EM
    0x00, // SUB
    0x00, // ESC
    0x00, // FS
    0x00, // GS
    0x00, // RS
    0x00, // US

    0x2c,         //  ' '
    0x1e | SHIFT, // !
    0x34 | SHIFT, // "
    0x20 | SHIFT, // #
    0x21 | SHIFT, // $
    0x22 | SHIFT, // %
    0x24 | SHIFT, // &
    0x34,         // '
    0x26 | SHIFT, // (
    0x27 | SHIFT, // )
    0x25 | SHIFT, // *
    0x2e | SHIFT, // +
    0x36,         // ,
    0x2d,         // -
    0x37,         // .
    0x38,         // /
    0x27,         // 0
    0x1e,         // 1
    0x1f,         // 2
    0x20,         // 3
    0x21,         // 4
    0x22,         // 5
    0x23,         // 6
    0x24,         // 7
    0x25,         // 8
    0x26,         // 9
    0x33 | SHIFT, // :
    0x33,         // ;
    0x36 | SHIFT, // <
    0x2e,         // =
    0x37 | SHIFT, // >
    0x38 | SHIFT, // ?
    0x1f | SHIFT, // @
    0x04 | SHIFT, // A
    0x05 | SHIFT, // B
    0x06 | SHIFT, // C
    0x07 | SHIFT, // D
    0x08 | SHIFT, // E
    0x09 | SHIFT, // F
    0x0a | SHIFT, // G
    0x0b | SHIFT, // H
    0x0c | SHIFT, // I
    0x0d | SHIFT, // J
    0x0e | SHIFT, // K
    0x0f | SHIFT, // L
    0x10 | SHIFT, // M
    0x11 | SHIFT, // N
    0x12 | SHIFT, // O
    0x13 | SHIFT, // P
    0x14 | SHIFT, // Q
    0x15 | SHIFT, // R
    0x16 | SHIFT, // S
    0x17 | SHIFT, // T
    0x18 | SHIFT, // U
    0x19 | SHIFT, // V
    0x1a | SHIFT, // W
    0x1b | SHIFT, // X
    0x1c | SHIFT, // Y
    0x1d | SHIFT, // Z
    0x2f,         // [
    0x31,         // bslash
    0x30,         // ]
    0x23 | SHIFT, // ^
    0x2d | SHIFT, // _
    0x35,         // `
    0x04,         // a
    0x05,         // b
    0x06,         // c
    0x07,         // d
    0x08,         // e
    0x09,         // f
    0x0a,         // g
    0x0b,         // h
    0x0c,         // i
    0x0d,         // j
    0x0e,         // k
    0x0f,         // l
    0x10,         // m
    0x11,         // n
    0x12,         // o
    0x13,         // p
    0x14,         // q
    0x15,         // r
    0x16,         // s
    0x17,         // t
    0x18,         // u
    0x19,         // v
    0x1a,         // w
    0x1b,         // x
    0x1c,         // y
    0x1d,         // z
    0x2f | SHIFT, // {
    0x31 | SHIFT, // |
    0x30 | SHIFT, // }
    0x35 | SHIFT, // ~
    0             // DEL
};

my_nkro_usb_keyboard_event_data_t MY_NKRO_USBKeyboard::get_keyboard_led_state()
{
    return _keyboard_led_state;
}

 bool MY_NKRO_USBKeyboard::hid_device_ready(){
    return hid.ready();
 }

size_t MY_NKRO_USBKeyboard::pressRaw(uint8_t k)
{
    if (k >= 0xE0 && k < 0xE8)
    {
        // it's a modifier key
        _keyReport.modifiers |= (1 << (k - 0xE0));
    }
    else if (k && k < 0x88)
    { // 只有当k在描述符设定的范围内时才处理，支持到hid键盘表中135号
        // k/8为要放到数组的位置，k%8为要左移的位数
        _keyReport.keys[(k / 8)] |= (1 << (k % 8));
    }
    else
    {
        // not a modifier and not a key
        return 0;
    }
    sendReport(&_keyReport);
    return 1;
}

size_t MY_NKRO_USBKeyboard::releaseRaw(uint8_t k)
{
    if (k >= 0xE0 && k < 0xE8)
    {
        // it's a modifier key
        _keyReport.modifiers &= ~(1 << (k - 0xE0));
    }
    else if (k && k < 0x88)
    {

        _keyReport.keys[(k / 8)] &= ~(1 << (k % 8));
    }
    else
    {
        // not a modifier and not a key
        return 0;
    }

    sendReport(&_keyReport);
    return 1;
}

// press() adds the specified key (printing, non-printing, or modifier)
// to the persistent key report and sends the report.  Because of the way
// USB HID works, the host acts like the key remains pressed until we
// call release(), releaseAll(), or otherwise clear the report and resend.
size_t MY_NKRO_USBKeyboard::press(uint8_t k)
{
    if (k >= 0x88)
    { // it's a non-printing key (not a modifier)
        // 非ascii内的数额外处理，用hid键盘表中的id+0x88重新定义
        k = k - 0x88;
    }
    else if (k >= 0x80)
    { // it's a modifier key
        _keyReport.modifiers |= (1 << (k - 0x80));
        k = 0;
    }
    else
    { // it's a printing key
        k = _asciimap[k];
        if (!k)
        {
            return 0;
        }
        if (k & 0x80) // 表中将键盘上的A、!这种键或0x80，其它键如果与0x80会是0
        {             // it's a capital letter or other character reached with shift
            // At boot, some PCs need a separate report with the shift key down like a real keyboard.
            if (shiftKeyReports)
            {
                pressRaw(HID_KEY_SHIFT_LEFT);
            }
            else
            {
                _keyReport.modifiers |= 0x02; // the left shift modifier
            }
            k &= 0x7F; // 把键还原
        }
    }
    return pressRaw(k);
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t MY_NKRO_USBKeyboard::release(uint8_t k)
{
    if (k >= 0x88)
    { // it's a non-printing key (not a modifier)
        k = k - 0x88;
    }
    else if (k >= 0x80)
    { // it's a modifier key
        _keyReport.modifiers &= ~(1 << (k - 0x80));
        k = 0;
    }
    else
    { // it's a printing key
        k = _asciimap[k];
        if (!k)
        {
            return 0;
        }
        if (k & 0x80)
        { // it's a capital letter or other character reached with shift
            if (shiftKeyReports)
            {
                releaseRaw(k & 0x7F);   // Release key without shift modifier
                k = HID_KEY_SHIFT_LEFT; // Below, release shift modifier
            }
            else
            {
                _keyReport.modifiers &= ~(0x02); // the left shift modifier
                k &= 0x7F;
            }
        }
    }
    return releaseRaw(k);
}

void MY_NKRO_USBKeyboard::releaseAll(void)
{
    memset(_keyReport.keys, 0, 17); // 根据报告中按键数组的大小设置要清零的字节数
    _keyReport.modifiers = 0;
    sendReport(&_keyReport);
}

size_t MY_NKRO_USBKeyboard::write(uint8_t c)
{
    uint8_t p = press(c); // Keydown
    release(c);           // Keyup
    return p;             // just return the result of press() since release() almost always returns 1
}

size_t MY_NKRO_USBKeyboard::write(const uint8_t *buffer, size_t size)
{
    size_t n = 0;
    while (size--)
    {
        if (*buffer != '\r')
        {
            if (write(*buffer))
            {
                n++;
            }
            else
            {
                break;
            }
        }
        buffer++;
    }
    return n;
}

#endif /* CONFIG_TINYUSB_HID_ENABLED */
#endif /* SOC_USB_OTG_SUPPORTED */