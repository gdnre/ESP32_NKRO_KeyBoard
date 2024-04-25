#include "my_esp32s3_configure.h"
#include "USB.h"
// #include <USBHIDMouse.h>
// #include "USBHIDKeyboard.h"
#include "USBHIDConsumerControl.h"
// #include "BleKeyboard.h"
#include "my_nkro_keyboard.h"
#include "my_nkro_BleKeyboard.h"

#define COLPINS_BITMASK 0x7c0
#define HIGHPIN_BITMASK(x) (1 << (x - 32))
#define EXKEYS_BITMASK 0x2800
#define ROWPINS_BITMASK 0x278000
#define WAKEUP_BUTTON_PIN_BITMASK 0x27A800


enum My_Key_Type
{
    empty_key = 0,       // 按键不进行任何操作
    single_key_code = 1, // 使用8位数字编码表示的按键
    single_char,         // 直接输入字符表示的按键
    consumer_code,       // 多媒体控制按键,16位数字
    combine_keys,        // 最多6个的组合按键
    fn_key,              // fn
    fn2_key,             // 第二个fn
    led_switch,          // 灯光模式切换
    screen_switch,       // 屏幕背光开关
    mode_switch          // 蓝牙有线切换
};

enum My_Key_State
{
    my_key_released = 0,
    my_key_pressed = 1,
    my_key_longpressed = 2 // 尽量别用
};

// 按键中可能存储的值,需要搭配my_key_type使用
union My_Key_Value
{
    uint8_t keycode;
    char char_key;
    uint16_t consumercode;
    uint8_t combinekeys[6];
    int8_t reserve;
};

// 存储按键值和状态的结构体
struct My_Virtual_Key
{
public:
    // 按键键值
    My_Key_Type key_type;
    My_Key_Value key_value;
    // 虚拟按键状态,结合fn使用,防止fn松开后按键仍然按住时,不松开按键
    My_Key_State pre_key_state;
    My_Virtual_Key()
    {
        key_type = My_Key_Type::empty_key;
        key_value.reserve = 0;
        pre_key_state = My_Key_State::my_key_released;
    }
    My_Virtual_Key(My_Key_Type _key_type) : My_Virtual_Key()
    {
        if (_key_type == My_Key_Type::single_key_code ||
            _key_type == My_Key_Type::single_char ||
            _key_type == My_Key_Type::consumer_code ||
            _key_type == My_Key_Type::combine_keys)
        {
            key_type = My_Key_Type::empty_key;
        }
        else
        {
            key_type = _key_type;
        }
    }
    My_Virtual_Key(uint8_t _key) : My_Virtual_Key()
    {
        key_type = My_Key_Type::single_key_code;
        key_value.keycode = _key;
    }
    My_Virtual_Key(char _key) : My_Virtual_Key()
    {
        key_type = My_Key_Type::single_char;
        key_value.char_key = _key;
    }
    My_Virtual_Key(uint16_t _key) : My_Virtual_Key()
    {
        key_type = My_Key_Type::consumer_code;
        key_value.consumercode = _key;
    }
    My_Virtual_Key(uint8_t _keys[], uint8_t size) : My_Virtual_Key()
    {
        key_type = My_Key_Type::combine_keys;
        for (uint8_t i = 0; i < 6; i++)
        {
            if (i >= size)
            {
                key_value.combinekeys[i] = 0;
            }
            else
            {
                key_value.combinekeys[i] = _keys[i];
            }
        }
    }
};

// 按键的物理位置的结构体，每个物理位置的按键可能有3层键值：原始层、fn层、fn2层，且至少包含原始层
struct My_Key_Slot
{
public:
    // 按键键值
    uint8_t id;
    uint8_t actual_key_num;         // 默认含原始按键(0),1为含fn层,2为含fn2层,3为含fn和fn2层
    My_Virtual_Key virtual_keys[3]; // 0为原始按键,1为fn层,2为fn2层
    // 按键状态,长按状态时,每200ms更新一次时间并发送key press
    // My_Key_State current_key_state;
    My_Key_State pre_slot_state;
    unsigned long pressed_time;
    // 方便配置用的构造函数
    My_Key_Slot()
    {
        id = 0;
        actual_key_num = 0;
        virtual_keys[0] = My_Virtual_Key();
        pre_slot_state = My_Key_State::my_key_released;
        pressed_time = 0;
    }
    My_Key_Slot(My_Key_Type _key_type) : My_Key_Slot()
    {
        virtual_keys[0] = My_Virtual_Key(_key_type);
    }
    My_Key_Slot(uint8_t _key) : My_Key_Slot()
    {
        virtual_keys[0] = My_Virtual_Key(_key);
    }
    My_Key_Slot(char _key) : My_Key_Slot()
    {
        virtual_keys[0] = My_Virtual_Key(_key);
    }
    My_Key_Slot(uint16_t _key) : My_Key_Slot()
    {
        virtual_keys[0] = My_Virtual_Key(_key);
    }
    My_Key_Slot(uint8_t _keys[], uint8_t size) : My_Key_Slot()
    {
        virtual_keys[0] = My_Virtual_Key(_keys, size);
    }
};

extern char usb_kb_name[];
extern char ble_kb_name[];

extern const MediaKeyReport MEDIAKEYRETURN0;

extern RTC_DATA_ATTR uint8_t connect_mode;
extern bool connect_mode_change;

extern uint8_t fn_state;

extern const uint8_t col_pins[];
extern const uint8_t col_num;
extern const uint8_t row_pins[];
extern const uint8_t row_num;

extern uint8_t ex_key_pins[];
#define EXKEY_COUNT 2

// 键盘矩阵物理按键id，没实际用到
extern uint8_t key_id_matrix[5][5];
extern uint8_t ex_key_id[];

// 组合键数组，可以随意增减，也可以自己定义新的数组
extern uint8_t my_combine_keys[][6];

// 额外定义fn键的位置,因为需要该位置在原始层,fn层,fn2层都被强制更改为fn(2),（不改貌似也可以，保险期间处理下）
// 坐标从0开始,任意-1表示不定义
extern const int fn_key_position[2];
extern const int fn2_key_position[2];

extern int8_t ex_key_fn;
extern int8_t ex_key_fn2;

// 键盘矩阵键值设置
extern RTC_DATA_ATTR My_Key_Slot key_slot_matrix[5][5];
// 键盘矩阵fn层
extern RTC_DATA_ATTR My_Virtual_Key key_value_matrix_fn[5][5];
// 键盘矩阵fn2层
extern RTC_DATA_ATTR My_Virtual_Key key_value_matrix_fn2[5][5];

// 额外按键设置
extern RTC_DATA_ATTR bool use_ex_keys;
extern uint8_t ex_key_pins[];

// 额外按键键值设置
extern RTC_DATA_ATTR My_Key_Slot ex_key_slot[];
// 额外按键fn层
extern RTC_DATA_ATTR My_Virtual_Key ex_key_value_fn[];
// 额外按键fn2层
extern RTC_DATA_ATTR My_Virtual_Key ex_key_value_fn2[];

// 编码器编码键设置
extern RTC_DATA_ATTR My_Key_Slot encoder_clockwise_keyslot;
extern RTC_DATA_ATTR My_Key_Slot encoder_anticlockwise_keyslot;
// 编码器fn层
extern RTC_DATA_ATTR My_Virtual_Key encoder_clockwise_fn;
extern RTC_DATA_ATTR My_Virtual_Key encoder_anticlockwise_fn;
// 编码器fn2层
extern RTC_DATA_ATTR My_Virtual_Key encoder_clockwise_fn2;
extern RTC_DATA_ATTR My_Virtual_Key encoder_anticlockwise_fn2;

// hid
extern USBHIDConsumerControl consumercontrol;
// extern USBHIDKeyboard keyboard;
extern  MY_NKRO_USBKeyboard keyboard;
// extern BleKeyboard blekeyboard;
extern NkroBleKeyboard blekeyboard;

// 键盘矩阵
// extern unsigned long keys_last_scan_time;

// 旋转编码器
// extern uint16_t encoder_counter;
extern uint8_t pre_ra;
extern uint8_t current_ra;
extern uint8_t current_rb;
extern uint8_t pre_rb;
extern uint8_t rb_change_sign;
extern unsigned long ra_low_sign_timer;
extern unsigned long rb_change_timer;
extern unsigned long encoder_last_trigger_timer;

// 函数声明
uint8_t is_second_key_different(My_Virtual_Key first_key, My_Virtual_Key second_key);
void set_key_slot(My_Key_Slot *key_slot, My_Virtual_Key fn, My_Virtual_Key fn2, uint8_t key_id = 0);
void init_key_pins();
void RTC_DATA_ATTR  init_key_slots();
//==================================================
void key_state_handle(My_Key_Slot *current_key, int pin_sign);
void send_my_key(My_Key_Value key_value, My_Key_Type key_type, bool isrelease = false, bool islongpress = false);
void scan_keys();
void scan_encoder();
void encoder_send_my_key(My_Key_Slot *current_key);
//=================================================
const MediaKeyReport &usbcode_to_blecode(uint16_t consumer_control_code);
//===================================
void change_kb_mode();

void start_keyboard();

void key_matrix_scan_task(void *pvParameters);

void encoder_scan_task(void *pvParameters);

void get_wake_up_keys();