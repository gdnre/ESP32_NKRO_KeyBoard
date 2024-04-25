#include "my_keyboard.h"
#include "my_rgb.h"
#include "my_display.h"

char usb_kb_name[] = "ESP32-S3 USB-KeyBoard";
char ble_kb_name[] = "ESP32-S3 BLE-KB";

const MediaKeyReport MEDIAKEYRETURN0 = {0, 0};

RTC_DATA_ATTR uint8_t connect_mode = 1; // 0蓝牙，1有线
bool connect_mode_change = false;

// 键盘fn状态存储
// 1表示按下fn,2表示按下fn2,0表示没按下fn,赋值时三种状态要互斥
uint8_t fn_state = 0;

// 键盘矩阵的引脚数组
const uint8_t col_pins[] = COL_PINS;
const uint8_t col_num = sizeof(col_pins) / sizeof(col_pins[0]);
const uint8_t row_pins[] = ROW_PINS;
const uint8_t row_num = sizeof(row_pins) / sizeof(row_pins[0]);

// 额外按键对应引脚
uint8_t ex_key_pins[] = {EX_KEY0, EX_KEY1};
#define EXKEY_COUNT 2

// 矩阵物理按键id，没实际用到
uint8_t key_id_matrix[5][5] = {
    {1, 6, 11, 16, 21},
    {2, 7, 12, 17, 22},
    {3, 8, 13, 18, 23},
    {4, 9, 14, 19, 24},
    {5, 10, 15, 20, 25}};

uint8_t ex_key_id[] = {26, 27};

/*==================================================================
========================自定义配置===============================
====================================================================*/
// 额外按键和屏幕二选一，1表示用按键,0表示用屏幕
RTC_DATA_ATTR bool use_ex_keys = true;

/*组合键，数组可以随意增减，也可以自己定义新的数组=======================*/
uint8_t my_combine_keys[][6] = {
    {KEY_NKRO_LEFT_CTRL, 'c'},
    {KEY_NKRO_LEFT_CTRL, 'v'}};

/*=========================fn键========================================*/
// 设置fn键的位置,因为需要该位置在原始层,fn层,fn2层都被强制更改为fn(2),（其实不额外设置应该也可以，保险起见处理下，在之后的初始化函数中进行处理）
// 键盘矩阵中的fn，坐标从0开始,任意-1表示不定义
const int fn_key_position[2] = {4, 0};
const int fn2_key_position[2] = {-1, -1};
// 额外按键中的fn，对应ex_key_pins数组，-1表示不设置为fn
int8_t ex_key_fn = -1;
int8_t ex_key_fn2 = -1;

/*按键配置说明：
    1.按键的键值通过自定义的结构体数组存储，配置格式为调用结构体的构造函数；
      1.1 按键结构体有2种,配置格式都一样：
            My_Key_Slot：可以理解为按键对应的物理位置，每个物理位置的按键根据fn状态，最多存储3个不同的键值（原始层、fn层、fn2层），内部其实是调用了My_Virtual_Key的构造函数赋值给原始层按键；
            My_Virtual_Key：存储1个按键的值，配置时根据按键不同，调用该结构体的不同构造函数初始化；
      1.2 按键类型：因为不同按键键值的数据结构不同，只能用一个包含所有按键数据结构的联合体My_Key_Value配合按键类型枚举My_Key_Type来存储，并使用构造函数来方便赋值；
    具体配置格式：
          ·My_Key_Slot()/My_Virtual_Key()：
              按键不进行任何操作，两个结构体格式一样，下面例子只写其中一个；
          ·My_Key_Slot((uint8_t)ascii数)：
              直接输入按键的数字代码，8位，如果定义时没指定为uint8_t，需要如示例进行类型转换，最好是填预先定义好的名称，不要直接填数字，预定义按键所在文件如下：
              ·BleKeyboard.h
              ·my_esp32s3_configure.h
              ·USBHIDKeyboard.h（和蓝牙键盘库中名称重复）
          ·My_Key_Slot('a')：
              直接填单个字符；
          ·My_Key_Slot((uint16_t)CONSUMER_CONTROL_XXXXX)：
              同上上，不过是16位数字，用于媒体控制等，预定义按键所在文件：
              ·USBHIDConsumerControl.h
          ·My_Key_Slot(my_combine_keys[i],size)：
              接收最多6个8位数字/字符的数组以及数组的长度，多于6个的会被舍弃，少于6个的会用0填充:
              ·my_combine_keys是我定义好的一个二维数组，方便配置用；
              ·也可以自己定义一个数组，或者使用指针；
          ·My_Key_Slot(My_Key_Type::empty_key):
              ·填按键类型时用作其它用途，比如fn，支持的按键类型：
                  ·My_Key_Type::empty_key
                  ·My_Key_Type::fn_key
                  ·My_Key_Type::fn2_key
                  ·My_Key_Type::led_switch
                  ·My_Key_Type::screen_switch
                  ·My_Key_Type::mode_switch
*/

/*=========================键盘矩阵=======================================*/
// 键盘矩阵键值设置
RTC_DATA_ATTR My_Key_Slot key_slot_matrix[5][5] = {
    {My_Key_Slot((uint8_t)KEY_NKRO_BACKSPACE),
     My_Key_Slot((uint8_t)KEY_KP_SLASH),
     My_Key_Slot((uint8_t)KEY_KP_ASTERISK),
     My_Key_Slot((uint16_t)CONSUMER_CONTROL_PLAY_PAUSE),
     My_Key_Slot(My_Key_Type::empty_key)},
    {My_Key_Slot((uint8_t)KEY_KP_7),
     My_Key_Slot((uint8_t)KEY_KP_8),
     My_Key_Slot((uint8_t)KEY_KP_9),
     My_Key_Slot((uint8_t)KEY_KP_MINUS),
     My_Key_Slot(My_Key_Type::empty_key)},
    {My_Key_Slot((uint8_t)KEY_KP_4),
     My_Key_Slot((uint8_t)KEY_KP_5),
     My_Key_Slot((uint8_t)KEY_KP_6),
     My_Key_Slot((uint8_t)KEY_KP_PLUS),
     My_Key_Slot(My_Key_Type::empty_key)},
    {My_Key_Slot((uint8_t)KEY_KP_1),
     My_Key_Slot((uint8_t)KEY_KP_2),
     My_Key_Slot((uint8_t)KEY_KP_3),
     My_Key_Slot((uint8_t)KEY_NKRO_RIGHT_ARROW),
     My_Key_Slot(My_Key_Type::empty_key)},
    {My_Key_Slot(My_Key_Type::fn_key),
     My_Key_Slot((uint8_t)KEY_KP_0),
     My_Key_Slot((uint8_t)KEY_KP_DOT),
     My_Key_Slot((uint8_t)KEY_KP_ENTER),
     My_Key_Slot(My_Key_Type::empty_key)}};
// 键盘矩阵fn层
RTC_DATA_ATTR My_Virtual_Key key_value_matrix_fn[5][5] = {
    {My_Virtual_Key(My_Key_Type::led_switch),
     My_Virtual_Key(My_Key_Type::mode_switch),
     My_Virtual_Key(My_Key_Type::screen_switch),
     My_Virtual_Key((uint16_t)CONSUMER_CONTROL_MUTE),
     My_Virtual_Key(My_Key_Type::empty_key)},
    {My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key)},
    {My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key)},
    {My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key)},
    {My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key)}};
// 键盘矩阵fn2层
RTC_DATA_ATTR My_Virtual_Key key_value_matrix_fn2[5][5] = {
    {My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key)},
    {My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key)},
    {My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key)},
    {My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key)},
    {My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key),
     My_Virtual_Key(My_Key_Type::empty_key)}};

/*=========================额外按键========================================*/
// 额外按键键值设置
RTC_DATA_ATTR My_Key_Slot ex_key_slot[] = {
    My_Key_Slot(my_combine_keys[0], 2),
    My_Key_Slot(my_combine_keys[1], 2)};
// 额外按键fn层
RTC_DATA_ATTR My_Virtual_Key ex_key_value_fn[] = {
    My_Virtual_Key(),
    My_Virtual_Key()};
// 额外按键fn2层
RTC_DATA_ATTR My_Virtual_Key ex_key_value_fn2[] = {
    My_Virtual_Key(),
    My_Virtual_Key()};

/*=========================旋转编码器========================================*/
// 编码器编码键设置
RTC_DATA_ATTR My_Key_Slot encoder_clockwise_keyslot = My_Key_Slot((uint16_t)CONSUMER_CONTROL_VOLUME_INCREMENT);
RTC_DATA_ATTR My_Key_Slot encoder_anticlockwise_keyslot = My_Key_Slot((uint16_t)CONSUMER_CONTROL_VOLUME_DECREMENT);
// 编码器fn层
RTC_DATA_ATTR My_Virtual_Key encoder_clockwise_fn = My_Virtual_Key((uint16_t)CONSUMER_CONTROL_SCAN_NEXT);
RTC_DATA_ATTR My_Virtual_Key encoder_anticlockwise_fn = My_Virtual_Key((uint16_t)CONSUMER_CONTROL_SCAN_PREVIOUS);
// 编码器fn2层
RTC_DATA_ATTR My_Virtual_Key encoder_clockwise_fn2;
RTC_DATA_ATTR My_Virtual_Key encoder_anticlockwise_fn2;
/*=========================按键配置结束========================================
=============================================================================
=============================================================================*/

// hid
USBHIDConsumerControl consumercontrol;
// USBHIDKeyboard keyboard;
MY_NKRO_USBKeyboard keyboard;
// BleKeyboard blekeyboard;
NkroBleKeyboard blekeyboard;

// 键盘矩阵
// unsigned long keys_last_scan_time = 0;

// 旋转编码器
// uint16_t encoder_counter = 0;
uint8_t pre_ra = 1;
uint8_t current_ra = 1;
uint8_t current_rb = 1;
uint8_t pre_rb = 1;
uint8_t rb_change_sign = 2;
unsigned long ra_low_sign_timer = 0;
unsigned long rb_change_timer = 0;
unsigned long encoder_last_trigger_timer = 0;

//=======================================函数===========================================
uint8_t is_second_key_different(My_Virtual_Key first_key, My_Virtual_Key second_key)
{
  // 将第二个键与第一个比较,如果第二个不为空键且与第一个不同,返回1,否则返回0
  if (second_key.key_type != My_Key_Type::empty_key)
  {
    if (second_key.key_type != first_key.key_type)
    {
      return 1;
    }
    else
    {
      switch (second_key.key_type)
      {
      case My_Key_Type::single_key_code:
        return first_key.key_value.keycode != second_key.key_value.keycode;
      case My_Key_Type::single_char:
        return first_key.key_value.char_key != second_key.key_value.char_key;
      case My_Key_Type::consumer_code:
        return first_key.key_value.consumercode != second_key.key_value.consumercode;
      case My_Key_Type::combine_keys:
        return first_key.key_value.combinekeys != second_key.key_value.combinekeys;
      default:
        break;
      }
      // return 0;
    }
  }
  return 0;
}

void set_key_slot(My_Key_Slot *key_slot, My_Virtual_Key fn, My_Virtual_Key fn2, uint8_t key_id)
{ // 根据按键配置设置单个按键
  if (key_id)
  {
    key_slot->id = key_id;
  }

  if (is_second_key_different(key_slot->virtual_keys[0], fn))
  {
    key_slot->virtual_keys[1] = fn;
    key_slot->actual_key_num |= 1;
  }
  if (is_second_key_different(key_slot->virtual_keys[0], fn2))
  {
    key_slot->virtual_keys[2] = fn2;
    key_slot->actual_key_num |= 2;
  }
  for (uint8_t n = 0; n < 3; n++)
  {
    if (key_slot->virtual_keys[n].key_type == My_Key_Type::fn_key || key_slot->virtual_keys[n].key_type == My_Key_Type::fn2_key)
    {
      key_slot->virtual_keys[n].key_type = My_Key_Type::empty_key;
    }
  }
}

void RTC_DATA_ATTR  init_key_pins()
{
  for (uint8_t i = 0; i < row_num; i++)
  {
    pinMode(row_pins[i], INPUT_PULLDOWN);
    for (uint8_t j = 0; j < col_num; j++)
    {
      pinMode(col_pins[j], OUTPUT);
    }
  }
  if (use_ex_keys)
  {
    for (uint8_t i = 0; i < EXKEY_COUNT; i++)
    {
      pinMode(ex_key_pins[i], INPUT_PULLDOWN);
    }
  }
  pinMode(RA, INPUT);
  pinMode(RB, INPUT);
}

void init_key_slots()
{ // 初始化按键引脚和按键键值
  // 根据fn和fn2矩阵设置按键插槽矩阵的内容
  for (uint8_t i = 0; i < row_num; i++)
  {
    pinMode(row_pins[i], INPUT_PULLDOWN);
    for (uint8_t j = 0; j < col_num; j++)
    {
      pinMode(col_pins[j], OUTPUT);
      set_key_slot(&key_slot_matrix[i][j], key_value_matrix_fn[i][j], key_value_matrix_fn2[i][j], key_id_matrix[i][j]);
    }
  }
  // 设置额外按键插槽的内容
  for (uint8_t i = 0; i < EXKEY_COUNT; i++)
  {
    if (use_ex_keys)
    {
      pinMode(ex_key_pins[i], INPUT_PULLDOWN);
    }
    set_key_slot(&ex_key_slot[i], ex_key_value_fn[i], ex_key_value_fn2[i], ex_key_id[i]);
  }
  // 设置旋转编码器编码内容
  pinMode(RA, INPUT);
  pinMode(RB, INPUT);
  set_key_slot(&encoder_clockwise_keyslot, encoder_clockwise_fn, encoder_clockwise_fn2);
  set_key_slot(&encoder_anticlockwise_keyslot, encoder_anticlockwise_fn, encoder_anticlockwise_fn2);

  // 设置fn和fn2键的位置
  if (fn2_key_position[0] >= 0 && fn2_key_position[1] >= 0 && fn2_key_position[0] < row_num && fn2_key_position[1] < col_num)
  {
    for (uint8_t i = 0; i < 3; i++)
    {
      key_slot_matrix[fn2_key_position[0]][fn2_key_position[1]].virtual_keys[i] = My_Virtual_Key(My_Key_Type::fn2_key);
    }
    key_slot_matrix[fn2_key_position[0]][fn2_key_position[1]].actual_key_num = 0;
  }
  if (fn_key_position[0] >= 0 && fn_key_position[1] >= 0 && fn_key_position[0] < row_num && fn_key_position[1] < col_num)
  {
    for (uint8_t i = 0; i < 3; i++)
    {
      key_slot_matrix[fn_key_position[0]][fn_key_position[1]].virtual_keys[i] = My_Virtual_Key(My_Key_Type::fn_key);
    }
    key_slot_matrix[fn2_key_position[0]][fn2_key_position[1]].actual_key_num = 0;
  }
  // 额外按键的fn位置
  if (ex_key_fn2 >= 0 && ex_key_fn2 < EXKEY_COUNT)
  {
    for (uint8_t i = 0; i < 3; i++)
    {
      ex_key_slot[ex_key_fn2].virtual_keys[i] = My_Virtual_Key(My_Key_Type::fn2_key);
    }
    ex_key_slot[ex_key_fn2].actual_key_num = 0;
  }
  if (ex_key_fn >= 0 && ex_key_fn < EXKEY_COUNT)
  {
    for (uint8_t i = 0; i < 3; i++)
    {
      ex_key_slot[ex_key_fn].virtual_keys[i] = My_Virtual_Key(My_Key_Type::fn_key);
    }
    ex_key_slot[ex_key_fn].actual_key_num = 0;
  }
}

//============================================================================================

void key_state_handle(My_Key_Slot *current_key, int pin_sign)
{
  // 扫描并获取按键状态
  if (pin_sign == HIGH)
  { // 高电平表示按键处于按下状态
    if (current_key->pre_slot_state == My_Key_State::my_key_released)
    { // 按键从释放到按下 先消抖 再发送按键
      // vTaskDelay(1 / portTICK_PERIOD_MS);
      if (millis() - current_key->pressed_time >= KEY_DEBOUNCING)
      { // 按位与获取当前需要按下哪层按键
        uint8_t whichfn = fn_state & current_key->actual_key_num & 3;
        // 先释放掉非当前层但处于按下状态的键
        for (uint8_t n = 0; n < 3; n++)
        {
          if (n == whichfn || current_key->virtual_keys[n].key_type == My_Key_Type::empty_key)
          {
            continue;
          }
          if (current_key->virtual_keys[n].pre_key_state != My_Key_State::my_key_released)
          {
            send_my_key(current_key->virtual_keys[n].key_value, current_key->virtual_keys[n].key_type, true);
            current_key->virtual_keys[n].pre_key_state = My_Key_State::my_key_released;
          }
        }
        // 按下当前层的按键
        send_my_key(current_key->virtual_keys[whichfn].key_value, current_key->virtual_keys[whichfn].key_type);
        current_key->virtual_keys[whichfn].pre_key_state = My_Key_State::my_key_pressed;
      }
      current_key->pre_slot_state = My_Key_State::my_key_pressed;
      current_key->pressed_time = millis();
    }
    else if (millis() - current_key->pressed_time >= SHORTPRESSTIME)
    {
      uint8_t whichfn = fn_state & current_key->actual_key_num & 3;
      // 按键长按时，如果fn状态和当前按下的按键层不对应，释放掉按键
      for (uint8_t n = 0; n < 3; n++)
      {
        if (n == whichfn || current_key->virtual_keys[n].key_type == My_Key_Type::empty_key)
        {
          continue;
        }
        if (current_key->virtual_keys[n].pre_key_state != My_Key_State::my_key_released)
        {
          send_my_key(current_key->virtual_keys[n].key_value, current_key->virtual_keys[n].key_type, true);
          current_key->virtual_keys[n].pre_key_state = My_Key_State::my_key_released;
        }
      }
      // 长按按键时隔一段时间重新尝试按下按键，一般没必要
      //  if (fn_state == 0 || current_key->actual_key_num == 0)
      //  {
      //    send_my_key(current_key->virtual_keys[0].key_value, current_key->virtual_keys[0].key_type, false, true);
      //    current_key->pre_slot_state = My_Key_State::my_key_longpressed;
      //    current_key->pressed_time = millis();
      //  }
    }
  }
  else if (pin_sign == LOW)
  { // 按键处于松开状态
    if (current_key->pre_slot_state != My_Key_State::my_key_released)
    { // 按键是从按下到松开
      for (uint8_t n = 0; n < 3; n++)
      {
        if ((current_key->virtual_keys[n].pre_key_state != My_Key_State::my_key_released) && (current_key->virtual_keys[n].key_type != My_Key_Type::empty_key))
        {
          send_my_key(current_key->virtual_keys[n].key_value, current_key->virtual_keys[n].key_type, true);
          current_key->virtual_keys[n].pre_key_state = My_Key_State::my_key_released;
        }
        if (current_key->actual_key_num == 0)
          break;
      }
      current_key->pre_slot_state = My_Key_State::my_key_released;
      current_key->pressed_time = millis();
    }
  }
}
//============================================================================================
// 主要按键逻辑处理===========================================================================
void send_my_key(My_Key_Value key_value, My_Key_Type key_type, bool isrelease, bool islongpress)
{ // 根据键值、类型和按下状态执行按键操作
  no_operate_timer = millis();
  switch (key_type)
  {
  case My_Key_Type::empty_key:
    break;
  case My_Key_Type::single_key_code:
    if (connect_mode == 1)
    {
      if (isrelease)
        keyboard.release(key_value.keycode);
      else
        keyboard.press(key_value.keycode);
    }
    else if (blekeyboard.isConnected())
    {
      if (isrelease)
        blekeyboard.release(key_value.keycode);
      else
        blekeyboard.press(key_value.keycode);
    }
    break;
  case My_Key_Type::single_char:
    if (connect_mode == 1)
    {
      if (isrelease)
        keyboard.release(key_value.char_key);
      else
        keyboard.press(key_value.char_key);
    }
    else if (blekeyboard.isConnected())
    {
      if (isrelease)
        blekeyboard.release(key_value.char_key);
      else
        blekeyboard.press(key_value.char_key);
    }
    break;
  case My_Key_Type::consumer_code:
    if (key_value.consumercode == CONSUMER_CONTROL_PLAY_PAUSE && islongpress)
      break;
    if (connect_mode == 1)
    {
      if (!isrelease)
      {
        consumercontrol.press(key_value.consumercode);
        consumercontrol.release();
      }
    }
    else if (blekeyboard.isConnected())
    {
      if (!isrelease)
      {
        blekeyboard.write(usbcode_to_blecode(key_value.consumercode));
      }
    }

    break;
  case My_Key_Type::combine_keys:
    if (islongpress)
    { // 组合按键不长按 且执行前后都释放掉所有按键
      break;
    }
    if (connect_mode == 1)
    {
      if (!isrelease)
      {
        keyboard.releaseAll();
        for (uint8_t i = 0; i < 6; i++)
        {
          if (key_value.combinekeys[i] != 0)
          {
            keyboard.press(key_value.combinekeys[i]);
          }
        }
        keyboard.releaseAll();
      }
    }
    else if (blekeyboard.isConnected())
    {
      if (!isrelease)
      {
        blekeyboard.releaseAll();
        for (uint8_t i = 0; i < 6; i++)
        {
          if (key_value.combinekeys[i] != 0)
          {
            blekeyboard.press(key_value.combinekeys[i]);
          }
        }
        blekeyboard.releaseAll();
      }
    }
    break;
  case My_Key_Type::fn_key:
    if (isrelease)
    {
      fn_state = 0;
    }
    else
    {
      fn_state = 1;
    }
    break;
  case My_Key_Type::fn2_key:
    if (isrelease)
    {
      fn_state = 0;
    }
    else
    {
      fn_state = 2;
    }
    break;
  case My_Key_Type::led_switch:
    if (!islongpress && !isrelease)
    {
      rgb_change = true;
    }
    break;
  case My_Key_Type::screen_switch:
    if (!islongpress && !isrelease)
    {
      screen_change = true;
      Serial.println("按下切换屏幕按键");
    }
    break;
  case My_Key_Type::mode_switch:
    if (!islongpress && !isrelease)
    {
      connect_mode_change = true;
      change_kb_mode();
    }
    break;
  default:
    break;
  }
}
//============================================================================================
//============================================================================================
void encoder_send_my_key(My_Key_Slot *current_key)
{
  uint8_t whichfn = fn_state & current_key->actual_key_num & 3;
  send_my_key(current_key->virtual_keys[whichfn].key_value, current_key->virtual_keys[whichfn].key_type);
  send_my_key(current_key->virtual_keys[whichfn].key_value, current_key->virtual_keys[whichfn].key_type, true);
}

void scan_keys()
{
  // 开始扫描,设置列引脚输出电平
  // unsigned long costtime = micros();
  // 键盘矩阵扫描
  for (uint8_t i = 0; i < col_num; i++)
  {
    for (uint8_t n = 0; n < col_num; n++)
    {
      if (n == i)
      {
        digitalWrite(col_pins[n], HIGH);
      }
      else
      {
        digitalWrite(col_pins[n], LOW);
      }
    }
    delayMicroseconds(50);
    for (uint8_t j = 0; j < row_num; j++)
    {
      // 遍历每行输入引脚，处理按键槽状态
      key_state_handle(&key_slot_matrix[j][i], digitalRead(row_pins[j]));
    }
  }

  // 额外按键扫描
  if (use_ex_keys)
  {
    for (uint8_t i = 0; i < EXKEY_COUNT; i++)
    {
      key_state_handle(&ex_key_slot[i], digitalRead(ex_key_pins[i]));
    }
  }
  // keys_last_scan_time = millis();
  // Serial.print("扫描用时");
  // Serial.println(micros() - costtime);
}

#ifdef ENCODER_SCAN_WITHOUT_DELAY
void scan_encoder()
{
  // if (millis()-encoder_last_trigger_timer < 20)
  // {
  //   // pre_ra = digitalRead(RA);
  //   return;
  // }
  current_ra = digitalRead(RA);
  if (pre_ra != current_ra)
  {
    if (current_ra == 0)
      ra_low_sign_timer = millis();
    pre_rb = 2;
    // else
    //   ra_high_sign_timer = millis();
  }
  if (current_ra == 0 && millis() - ra_low_sign_timer > 1)
  {
    if (pre_rb == 2)
    {
      pre_rb = digitalRead(RB);
    }
    if (millis() - ra_low_sign_timer > 2 && pre_rb == digitalRead(RB))
    {
      // encoder_counter++;
      if (pre_rb == 1)
      {
        // Serial.print("顺时针");
        encoder_send_my_key(&encoder_clockwise_keyslot);
      }
      else
      {
        // Serial.print("逆时针");
        encoder_send_my_key(&encoder_anticlockwise_keyslot);
      }
      // Serial.println(encoder_counter);
      encoder_last_trigger_timer = millis();
      pre_rb = 3;
    }
  }
  pre_ra = digitalRead(RA);
}
#else
void scan_encoder()
{
  current_ra = digitalRead(RA);
  if (current_ra == 0 && pre_ra != current_ra)
  { // ra上升沿读取数据 2ms消抖
    vTaskDelay(2 / portTICK_PERIOD_MS);
    if (digitalRead(RA) == 0)
    {
      pre_rb = digitalRead(RB);
      vTaskDelay(1 / portTICK_PERIOD_MS);
      if (pre_rb == 1 && pre_rb == digitalRead(RB))
      { // 顺时针
        encoder_send_my_key(&encoder_clockwise_keyslot);
      }
      else if (pre_rb == 0 && pre_rb == digitalRead(RB))
      { // 逆时针
        encoder_send_my_key(&encoder_anticlockwise_keyslot);
      }
    }
  }
  pre_ra = digitalRead(RA);
}
#endif

//==============================================================================================
const MediaKeyReport &usbcode_to_blecode(uint16_t consumer_control_code)
{
  switch (consumer_control_code)
  {
  case CONSUMER_CONTROL_PLAY_PAUSE:
    return KEY_MEDIA_PLAY_PAUSE;
  case CONSUMER_CONTROL_VOLUME_INCREMENT:
    return KEY_MEDIA_VOLUME_UP;
  case CONSUMER_CONTROL_VOLUME_DECREMENT:
    return KEY_MEDIA_VOLUME_DOWN;
  case CONSUMER_CONTROL_SCAN_NEXT:
    return KEY_MEDIA_NEXT_TRACK;
  case CONSUMER_CONTROL_SCAN_PREVIOUS:
    return KEY_MEDIA_PREVIOUS_TRACK;
  case CONSUMER_CONTROL_MUTE:
    return KEY_MEDIA_MUTE;
  case CONSUMER_CONTROL_STOP:
    return KEY_MEDIA_STOP;
  case CONSUMER_CONTROL_HOME:
    return KEY_MEDIA_WWW_HOME;
  case CONSUMER_CONTROL_LOCAL_BROWSER:
    return KEY_MEDIA_LOCAL_MACHINE_BROWSER;
  case CONSUMER_CONTROL_CALCULATOR:
    return KEY_MEDIA_CALCULATOR;
  case CONSUMER_CONTROL_BOOKMARKS:
    return KEY_MEDIA_WWW_BOOKMARKS;
  case CONSUMER_CONTROL_SEARCH:
    return KEY_MEDIA_WWW_SEARCH;
  case CONSUMER_CONTROL_BR_STOP:
    return KEY_MEDIA_WWW_STOP;
  case CONSUMER_CONTROL_BACK:
    return KEY_MEDIA_WWW_BACK;
  case CONSUMER_CONTROL_CONFIGURATION:
    return KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION;
  case CONSUMER_CONTROL_EMAIL_READER:
    return KEY_MEDIA_EMAIL_READER;
  default:
    break;
  }
  return MEDIAKEYRETURN0;
}

//=======================================================
// #define DONT_RESTART_WHEN_CHANGE_MODE 1
// 蓝牙键盘断开后不自动重连，靠重启重连
void start_keyboard()
{
  if (connect_mode == 0)
  {
    blekeyboard.setName(ble_kb_name);
    blekeyboard.begin();
  }
  else
  {
    USB.productName(usb_kb_name);
    keyboard.begin();
    consumercontrol.begin();
    USB.begin();
  }
}

#ifdef DONT_RESTART_WHEN_CHANGE_MODE
void change_kb_mode()
{
  if (connect_mode_change)
  {
    if (connect_mode == 0) // 当前是蓝牙模式
    {
      blekeyboard.releaseAll();
      blekeyboard.end();
      connect_mode = 1;
      keyboard.begin();
      consumercontrol.begin();
      USB.begin();
      Serial.println("已切换为有线模式");
    }
    else // if(connect_mode == 1)
    {
      consumercontrol.release();
      consumercontrol.end();
      keyboard.releaseAll();
      keyboard.end();
      connect_mode = 0;
      // blekeyboard = BleKeyboard(ble_kb_name);
      blekeyboard.begin();
      Serial.println("已切换为蓝牙模式");
      Serial.println(blekeyboard.isConnected());
    }
    my_config_manager.begin(default_config_name);
    my_config_manager.putUChar("connect_mode", connect_mode);
    my_config_manager.end();
    Serial.println("更改连接模式配置成功");
  }
}
#else
void change_kb_mode()
{
  if (connect_mode_change)
  {
    if (connect_mode == 0) // 当前是蓝牙模式
    {
      blekeyboard.releaseAll();
      connect_mode = 1;
    }
    else // if(connect_mode == 1)
    {
      consumercontrol.release();
      keyboard.releaseAll();
      connect_mode = 0;
    }
    my_config_manager.begin(default_config_name);
    my_config_manager.putUChar("connect_mode", connect_mode);
    my_config_manager.end();
    Serial.println("更改连接模式配置成功,重启");
    esp_restart();
  }
}
#endif

void keyboard_led_event_callback(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{ // led状态改变时执行的函数
  Serial.println();
  Serial.print(event_base);
  Serial.print(":");
  Serial.println(event_id);
  uint8_t c_kb_led = *(uint8_t *)event_data;
  Serial.printf("numlock     capslock    scrolllock  compose     kana\n");
  //  numlock     capslock    scrolllock  compose     kana
  //  on          on          on          on          on
  for (size_t i = 0; i < 5; i++)
  {
    if (((c_kb_led & 0xff) >> i))
    {
      Serial.print("on          ");
    }
    else
    {
      Serial.print("off         ");
    }
  }
  Serial.printf("\n\n");
}

void key_matrix_scan_task(void *pvParameters)
{
  char *taskname = pcTaskGetName(NULL);
  int16_t freestack = uxTaskGetStackHighWaterMark(NULL);
  Serial.printf("任务<%s>开始运行，剩余堆栈为%d\n", taskname, freestack);

  TickType_t xLastWakeTime_scan_key;
  const TickType_t xFrequeency_scan = KEY_SCAN_SPEED;
  // BaseType_t xWasDelayed;
  init_key_slots();
  keyboard.onEvent(MY_NKRO_USB_KEYBOARD_LED_EVENT, keyboard_led_event_callback);

  for (;;)
  {
    scan_keys();
    // xWasDelayed =
    xTaskDelayUntil(&xLastWakeTime_scan_key, xFrequeency_scan);
    // vTaskDelay(KEY_SCAN_SPEED / portTICK_PERIOD_MS);
  }
}

void encoder_scan_task(void *pvParameters)
{
  char *taskname = pcTaskGetName(NULL);
  int16_t freestack = uxTaskGetStackHighWaterMark(NULL);
  Serial.printf("任务<%s>开始运行，剩余堆栈为%d\n", taskname, freestack);

  TickType_t xLastWakeTime_encoder;
  const TickType_t xFrequeency_encoder = 1;

  for (;;)
  {
    scan_encoder();
    // vTaskDelay(1 / portTICK_PERIOD_MS);
    xTaskDelayUntil(&xLastWakeTime_encoder, xFrequeency_encoder);
  }
}

void get_wake_up_keys()
{ // 从深睡唤醒键盘时可以获取到具体哪个键触发的，但usb启动太慢了，延迟太高还不如不发送按键
  if (stub_high_level_pins & EXKEYS_BITMASK)
  {
    if (use_ex_keys)
    {
      for (uint8_t i = 0; i < EXKEY_COUNT; i++)
      {
        if ((1 << ex_key_pins[i]) & stub_high_level_pins)
        {
          key_state_handle(&ex_key_slot[i], HIGH);
        }
      }
    }
  }
  if (stub_high_level_pins & ROWPINS_BITMASK)
  {
    for (uint8_t i = 0; i < col_num; i++)
    {
      for (uint8_t j = 0; j < row_num; j++)
      {
        if (stud_scan_result[i] & (1 << row_pins[j]))
        {
          keyboard.write(key_slot_matrix[j][i].virtual_keys->key_value.keycode);
        }
      }
      stud_scan_result[i] = 0;
    }
  }
  stub_high_level_pins = 0;
}