#include <Arduino.h>
#include "Preferences.h"

#ifndef MY_CONFIGURE
#define MY_CONFIGURE 1
// 键盘矩阵
/*布局示意图            out
 左上角 col1    col2    col3    col4    (col5)    扩展按键1   扩展按键2
    row1                       编码器   空
    row2                                空
in  row3                                空
    row4                                空
    row5                                空
*/
// 键盘矩阵列引脚,输出,默认上拉
#define COL0 39
#define COL1 40
#define COL2 41
#define COL3 42
#define COL4 38
#define COL_PINS           \
    {                      \
        39, 40, 41, 42, 38 \
    }

// 键盘矩阵行引脚,输入,默认下拉
#define ROW0 21
#define ROW1 18
#define ROW2 17
#define ROW3 16
#define ROW4 15
#define ROW_PINS           \
    {                      \
        21, 18, 17, 16, 15 \
    }

// 扩展按键,屏幕引脚共用,输入,高电平有效
#define EX_KEY0 11
#define EX_KEY1 13

// 旋转编码器引脚
#define RA 1
#define RB 2

// 屏幕引脚
#define LCD_SDA 11
#define LCD_DC 12
#define LCD_CS 37
#define LCD_RES 36
#define LCD_SCL 13
#define LCD_BL 47

// LED引脚
// LED电源(控制亮度)
#define LED_POWER 48
// LED数据输入
#define LED_DIN 45
#define LED_COUNT 21

// 其它扩展引脚,可随意使用,或者按照特定功能使用
// 摇杆引脚,adc引脚在模组引脚处有接电容
#define MY_ADC0 6
#define MY_ADC1 7
#define ROCKER_KEY 35

// 触摸引脚
#define MY_TOUCH0 10
#define MY_TOUCH1 14
#define MY_TOUCH2 5
#define MY_TOUCH3 4

// i2c
#define MY_SDA 8
#define MY_SCL 9

// 串口
#define MY_RX0 44
#define MY_TX0 43

// strapping引脚
#define MY_BOOT 0
#define MY_LOGMODE 3
#define MY_BOOT_MODE_CONTROL2 46

// usb引脚,未引出
// #define D正 20 //全速设备d+上拉
// #define D负 19

// 引脚原功能组
// rtc引脚
#define RTC_PINS                                                                     \
    {                                                                                \
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 \
    }
// adc引脚,adc功能优先使用组2,组1和无线功能可能有冲突
#define ADC_PINS1                        \
    {                                    \
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 \
    }
#define ADC_PINS2                              \
    {                                          \
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20 \
    }

// 触摸引脚
#define TOUCH_PINS                                       \
    {                                                    \
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 \
    }


// 额外按键定义
#define KEY_PRINT_SCREEN 0xCE
#define KEY_SCROLL_LOCK 0xCF
#define KEY_PAUSE 0xD0
// 数字小键盘
#define KEY_NUM_LOCK 0xDB
#define KEY_KP_SLASH 0xDC
#define KEY_KP_ASTERISK 0xDD
#define KEY_KP_MINUS 0xDE
#define KEY_KP_PLUS 0xDF
#define KEY_KP_ENTER 0xE0
#define KEY_KP_1 0xE1
#define KEY_KP_2 0xE2
#define KEY_KP_3 0xE3
#define KEY_KP_4 0xE4
#define KEY_KP_5 0xE5
#define KEY_KP_6 0xE6
#define KEY_KP_7 0xE7
#define KEY_KP_8 0xE8
#define KEY_KP_9 0xE9
#define KEY_KP_0 0xEA
#define KEY_KP_DOT 0xEB

#define KEY_DEBOUNCING 10
#define SHORTPRESSTIME 150 // 短按按住时不响应时间 单位ms

#define KEY_SCAN_SPEED 1 // 按键扫描间隔 单位ms

#define USED_PINS                                                                                                  \
    {                                                                                                              \
        39, 40, 41, 42, 38, 21, 18, 17, 16, 15, 1, 2, 11, 12, 37, 36, 13, 47, 48, 45, 6, 7, 35, 10, 14, 5, 4, 8, 9 \
    }

#endif

extern const char default_config_name[];
extern Preferences my_config_manager;

extern uint8_t used_pins[];
extern uint8_t pins_num;

extern bool use_ex_keys;

extern RTC_DATA_ATTR int bootCount;
extern RTC_DATA_ATTR uint32_t stub_high_level_pins;
extern RTC_DATA_ATTR uint32_t stud_scan_result[5];

extern unsigned long no_operate_timer;
extern const bool enable_deep_sleep;
extern unsigned long deep_sleep_time;

extern uint32_t keyscan_interrupt_flag;
extern uint32_t encoder_interrupt_flag;

extern TaskHandle_t *keyboard_task_handle;
extern TaskHandle_t *encoder_task_handle;
extern TaskHandle_t *rgb_task_handle;
extern TaskHandle_t *screen_task_handle;

// 将大部分引脚设置为输入模式，防止漏电
void Set_Pins_To_Input(uint8_t pins[], uint8_t pins_num);
// 启动前获取配置
void init_configs();

// 没有配置时新建配置，可以认为一共只执行1次
void create_configs();
void get_configs();

void deep_sleep_check_task(void *pvParameters);
// void light_sleep_check_task(void *pvParameters);

void keyscan_interrupt_isr_callback();
void encoder_interrupt_isr_callback();
