#include "my_nkro_BleKeyboard.h"

#if defined(USE_NIMBLE)
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLEHIDDevice.h>
#else
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#endif // USE_NIMBLE
#include "HIDTypes.h"
#include <driver/adc.h>
#include "sdkconfig.h"

#if defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define LOG_TAG ""
#else
#include "esp_log.h"
static const char *LOG_TAG = "BLEDevice";
#endif

// Report IDs:
#define MY_NKRO_BLEKEYBOARD_ID 0x64
#define MY_NKRO_MEDIA_KEYS_ID 0x65

static const uint8_t _hidReportDescriptor[] = {
	0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
	0x09, 0x06, // Usage (Keyboard)
	0xA1, 0x01, // Collection (Application)

	0x85, MY_NKRO_BLEKEYBOARD_ID, //   Report ID (100)
	0x05, 0x07,					  //   Usage Page (Kbrd/Keypad)
	0x15, 0x00,					  //   Logical Minimum (0)
	0x25, 0x01,					  //   Logical Maximum (1)

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
	// ------------------------------------------------- Media Keys
	USAGE_PAGE(1), 0x0C,				 // USAGE_PAGE (Consumer)
	USAGE(1), 0x01,						 // USAGE (Consumer Control)
	COLLECTION(1), 0x01,				 // COLLECTION (Application)
	REPORT_ID(1), MY_NKRO_MEDIA_KEYS_ID, //   REPORT_ID (101)
	USAGE_PAGE(1), 0x0C,				 //   USAGE_PAGE (Consumer)
	LOGICAL_MINIMUM(1), 0x00,			 //   LOGICAL_MINIMUM (0)
	LOGICAL_MAXIMUM(1), 0x01,			 //   LOGICAL_MAXIMUM (1)
	REPORT_SIZE(1), 0x01,				 //   REPORT_SIZE (1)
	REPORT_COUNT(1), 0x10,				 //   REPORT_COUNT (16)
	USAGE(1), 0xB5,						 //   USAGE (Scan Next Track)     ; bit 0: 1
	USAGE(1), 0xB6,						 //   USAGE (Scan Previous Track) ; bit 1: 2
	USAGE(1), 0xB7,						 //   USAGE (Stop)                ; bit 2: 4
	USAGE(1), 0xCD,						 //   USAGE (Play/Pause)          ; bit 3: 8
	USAGE(1), 0xE2,						 //   USAGE (Mute)                ; bit 4: 16
	USAGE(1), 0xE9,						 //   USAGE (Volume Increment)    ; bit 5: 32
	USAGE(1), 0xEA,						 //   USAGE (Volume Decrement)    ; bit 6: 64
	USAGE(2), 0x23, 0x02,				 //   Usage (WWW Home)            ; bit 7: 128
	USAGE(2), 0x94, 0x01,				 //   Usage (My Computer) ; bit 0: 1
	USAGE(2), 0x92, 0x01,				 //   Usage (Calculator)  ; bit 1: 2
	USAGE(2), 0x2A, 0x02,				 //   Usage (WWW fav)     ; bit 2: 4
	USAGE(2), 0x21, 0x02,				 //   Usage (WWW search)  ; bit 3: 8
	USAGE(2), 0x26, 0x02,				 //   Usage (WWW stop)    ; bit 4: 16
	USAGE(2), 0x24, 0x02,				 //   Usage (WWW back)    ; bit 5: 32
	USAGE(2), 0x83, 0x01,				 //   Usage (Media sel)   ; bit 6: 64
	USAGE(2), 0x8A, 0x01,				 //   Usage (Mail)        ; bit 7: 128
	HIDINPUT(1), 0x02,					 //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	END_COLLECTION(0)					 // END_COLLECTION
};

NkroBleKeyboard::NkroBleKeyboard(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel)
	: hid(0), deviceName(std::string(deviceName).substr(0, 15)), deviceManufacturer(std::string(deviceManufacturer).substr(0, 15)), batteryLevel(batteryLevel) {}

void NkroBleKeyboard::begin(void)
{
	BLEDevice::init(deviceName);
	BLEServer *pServer = BLEDevice::createServer();
	pServer->setCallbacks(this);
	hid = new BLEHIDDevice(pServer);
	inputKeyboard = hid->inputReport(MY_NKRO_BLEKEYBOARD_ID); // <-- input REPORTID from report map
	outputKeyboard = hid->outputReport(MY_NKRO_BLEKEYBOARD_ID);
	inputMediaKeys = hid->inputReport(MY_NKRO_MEDIA_KEYS_ID);

	outputKeyboard->setCallbacks(this);

	hid->manufacturer()->setValue(deviceManufacturer);

	hid->pnp(0x02, vid, pid, version);
	hid->hidInfo(0x00, 0x01);

#if defined(USE_NIMBLE)

	BLEDevice::setSecurityAuth(true, true, true);

#else

	BLESecurity *pSecurity = new BLESecurity();
	pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);

#endif // USE_NIMBLE

	hid->reportMap((uint8_t *)_hidReportDescriptor, sizeof(_hidReportDescriptor));
	hid->startServices();

	onStarted(pServer);

	advertising = pServer->getAdvertising();
	advertising->setAppearance(HID_KEYBOARD); // 标记，不确定是否会冲突
	advertising->addServiceUUID(hid->hidService()->getUUID());
	advertising->setScanResponse(false);
	advertising->start();
	hid->setBatteryLevel(batteryLevel);

	ESP_LOGD(LOG_TAG, "Advertising started!");
}

void NkroBleKeyboard::end(void)
{
}

bool NkroBleKeyboard::isConnected(void)
{
	return this->connected;
}

void NkroBleKeyboard::setBatteryLevel(uint8_t level)
{
	this->batteryLevel = level;
	if (hid != 0)
		this->hid->setBatteryLevel(this->batteryLevel);
}

// must be called before begin in order to set the name
void NkroBleKeyboard::setName(std::string deviceName)
{
	this->deviceName = deviceName;
}

/**
 * @brief Sets the waiting time (in milliseconds) between multiple keystrokes in NimBLE mode.
 *
 * @param ms Time in milliseconds
 */
void NkroBleKeyboard::setDelay(uint32_t ms)
{
	this->_delay_ms = ms;
}

void NkroBleKeyboard::set_vendor_id(uint16_t vid)
{
	this->vid = vid;
}

void NkroBleKeyboard::set_product_id(uint16_t pid)
{
	this->pid = pid;
}

void NkroBleKeyboard::set_version(uint16_t version)
{
	this->version = version;
}

void NkroBleKeyboard::sendReport(NkroBLEKeyReport *keys)
{
	if (this->isConnected())
	{
		this->inputKeyboard->setValue((uint8_t *)keys, sizeof(NkroBLEKeyReport));
		this->inputKeyboard->notify();
#if defined(USE_NIMBLE)
		// vTaskDelay(delayTicks);
		this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
	}
}

void NkroBleKeyboard::sendReport(nkro_MediaKeyReport *keys)
{
	if (this->isConnected())
	{
		this->inputMediaKeys->setValue((uint8_t *)keys, sizeof(nkro_MediaKeyReport));
		this->inputMediaKeys->notify();
#if defined(USE_NIMBLE)
		// vTaskDelay(delayTicks);
		this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
	}
}

extern const uint8_t _ble_asciimap[128] PROGMEM;

#ifndef SHIFT
#define SHIFT 0x80
#endif
const uint8_t _ble_asciimap[128] =
	{
		0x00, // NUL
		0x00, // SOH
		0x00, // STX
		0x00, // ETX
		0x00, // EOT
		0x00, // ENQ
		0x00, // ACK
		0x00, // BEL
		0x2a, // BS	Backspace
		0x2b, // TAB	Tab
		0x28, // LF	Enter
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

		0x2c,		  //  ' '
		0x1e | SHIFT, // !
		0x34 | SHIFT, // "
		0x20 | SHIFT, // #
		0x21 | SHIFT, // $
		0x22 | SHIFT, // %
		0x24 | SHIFT, // &
		0x34,		  // '
		0x26 | SHIFT, // (
		0x27 | SHIFT, // )
		0x25 | SHIFT, // *
		0x2e | SHIFT, // +
		0x36,		  // ,
		0x2d,		  // -
		0x37,		  // .
		0x38,		  // /
		0x27,		  // 0
		0x1e,		  // 1
		0x1f,		  // 2
		0x20,		  // 3
		0x21,		  // 4
		0x22,		  // 5
		0x23,		  // 6
		0x24,		  // 7
		0x25,		  // 8
		0x26,		  // 9
		0x33 | SHIFT, // :
		0x33,		  // ;
		0x36 | SHIFT, // <
		0x2e,		  // =
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
		0x2f,		  // [
		0x31,		  // bslash
		0x30,		  // ]
		0x23 | SHIFT, // ^
		0x2d | SHIFT, // _
		0x35,		  // `
		0x04,		  // a
		0x05,		  // b
		0x06,		  // c
		0x07,		  // d
		0x08,		  // e
		0x09,		  // f
		0x0a,		  // g
		0x0b,		  // h
		0x0c,		  // i
		0x0d,		  // j
		0x0e,		  // k
		0x0f,		  // l
		0x10,		  // m
		0x11,		  // n
		0x12,		  // o
		0x13,		  // p
		0x14,		  // q
		0x15,		  // r
		0x16,		  // s
		0x17,		  // t
		0x18,		  // u
		0x19,		  // v
		0x1a,		  // w
		0x1b,		  // x
		0x1c,		  // y
		0x1d,		  // z
		0x2f | SHIFT, // {
		0x31 | SHIFT, // |
		0x30 | SHIFT, // }
		0x35 | SHIFT, // ~
		0			  // DEL
};

uint8_t USBPutChar(uint8_t c);

// press() adds the specified key (printing, non-printing, or modifier)
// to the persistent key report and sends the report.  Because of the way
// USB HID works, the host acts like the key remains pressed until we
// call release(), releaseAll(), or otherwise clear the report and resend.
size_t NkroBleKeyboard::press(uint8_t k)
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
		k = pgm_read_byte(_ble_asciimap + k);
		if (!k)
		{
			setWriteError();
			return 0;
		}
		if (k & 0x80)
		{								  // it's a capital letter or other character reached with shift
			_keyReport.modifiers |= 0x02; // the left shift modifier
			k &= 0x7F;
		}
	}
	// Add k to the key report only if it's not already present
	// and if there is an empty slot.
	if (k && k < 0x88)
	{
		_keyReport.keys[(k / 8)] |= (1 << (k % 8));
	}
	else
	{
		// not a modifier and not a key
		setWriteError();
		return 0;
	}
	sendReport(&_keyReport);
	return 1;
}

size_t NkroBleKeyboard::press(const nkro_MediaKeyReport k)
{
	uint16_t k_16 = k[1] | (k[0] << 8);
	uint16_t mediaKeyReport_16 = _mediaKeyReport[1] | (_mediaKeyReport[0] << 8);

	mediaKeyReport_16 |= k_16;
	_mediaKeyReport[0] = (uint8_t)((mediaKeyReport_16 & 0xFF00) >> 8);
	_mediaKeyReport[1] = (uint8_t)(mediaKeyReport_16 & 0x00FF);

	sendReport(&_mediaKeyReport);
	return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t NkroBleKeyboard::release(uint8_t k)
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
		k = pgm_read_byte(_ble_asciimap + k);
		if (!k)
		{
			return 0;
		}
		if (k & 0x80)
		{									 // it's a capital letter or other character reached with shift
			_keyReport.modifiers &= ~(0x02); // the left shift modifier
			k &= 0x7F;
		}
	}

	// Test the key report to see if k is present.  Clear it if it exists.
	// Check all positions in case the key is present more than once (which it shouldn't be)
	if (k && k < 0x88)
	{

		_keyReport.keys[(k / 8)] &= ~(1 << (k % 8));
	}

	sendReport(&_keyReport);
	return 1;
}

size_t NkroBleKeyboard::release(const nkro_MediaKeyReport k)
{
	uint16_t k_16 = k[1] | (k[0] << 8);
	uint16_t mediaKeyReport_16 = _mediaKeyReport[1] | (_mediaKeyReport[0] << 8);
	mediaKeyReport_16 &= ~k_16;
	_mediaKeyReport[0] = (uint8_t)((mediaKeyReport_16 & 0xFF00) >> 8);
	_mediaKeyReport[1] = (uint8_t)(mediaKeyReport_16 & 0x00FF);

	sendReport(&_mediaKeyReport);
	return 1;
}

void NkroBleKeyboard::releaseAll(void)
{
	memset(_keyReport.keys, 0, 17); // 根据报告中按键数组的大小设置要清零的字节数
	_keyReport.modifiers = 0;
	_mediaKeyReport[0] = 0;
	_mediaKeyReport[1] = 0;
	sendReport(&_keyReport);
}

size_t NkroBleKeyboard::write(uint8_t c)
{
	uint8_t p = press(c); // Keydown
	release(c);			  // Keyup
	return p;			  // just return the result of press() since release() almost always returns 1
}

size_t NkroBleKeyboard::write(const nkro_MediaKeyReport c)
{
	uint16_t p = press(c); // Keydown
	release(c);			   // Keyup
	return p;			   // just return the result of press() since release() almost always returns 1
}

size_t NkroBleKeyboard::write(const uint8_t *buffer, size_t size)
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

void NkroBleKeyboard::register_connect_event(ble_connect_event_cb_t eventCb)
{
	this->connect_cb = eventCb;
}

void NkroBleKeyboard::onConnect(BLEServer *pServer)
{
	this->connected = true;
	if (this->connect_cb != NULL)
	{
		this->connect_cb(this->connected);
	}

#if !defined(USE_NIMBLE)

	BLE2902 *desc = (BLE2902 *)this->inputKeyboard->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
	desc->setNotifications(true);
	desc = (BLE2902 *)this->inputMediaKeys->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
	desc->setNotifications(true);

#endif // !USE_NIMBLE
}

void NkroBleKeyboard::onDisconnect(BLEServer *pServer)
{
	this->connected = false;
	if (this->connect_cb != NULL)
	{
		this->connect_cb(this->connected);
	}
#if !defined(USE_NIMBLE)

	BLE2902 *desc = (BLE2902 *)this->inputKeyboard->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
	desc->setNotifications(false);
	desc = (BLE2902 *)this->inputMediaKeys->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
	desc->setNotifications(false);

	advertising->start();

#endif // !USE_NIMBLE
}

void NkroBleKeyboard::onWrite(BLECharacteristic *me)
{
	uint8_t *value = (uint8_t *)(me->getValue().c_str());
	(void)value;
	ESP_LOGI(LOG_TAG, "special keys: %d", *value);
}

void NkroBleKeyboard::delay_ms(uint64_t ms)
{
	uint64_t m = esp_timer_get_time();
	if (ms)
	{
		uint64_t e = (m + (ms * 1000));
		if (m > e)
		{ // overflow
			while (esp_timer_get_time() > e)
			{
			}
		}
		while (esp_timer_get_time() < e)
		{
		}
	}
}