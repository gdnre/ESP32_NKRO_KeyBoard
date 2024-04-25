#include "my_esp32s3_configure.h"
#include "my_keyboard.h"
#include "my_display.h"
#include "my_rgb.h"


const char default_config_name[] = "config_0";
Preferences my_config_manager;

uint8_t used_pins[] = USED_PINS;
uint8_t pins_num = sizeof(used_pins) / sizeof(used_pins[0]);

uint32_t keyscan_interrupt_flag = 1;
uint32_t encoder_interrupt_flag = 1;

void Set_Pins_To_Input(uint8_t pins[], uint8_t pins_num)
{
    for (uint8_t i = 0; i < pins_num; i++)
    {
        pinMode(pins[i], INPUT);
    }
    return;
}

void create_configs()
{
    if (!my_config_manager.isKey("connect_mode"))
        my_config_manager.putUChar("connect_mode", connect_mode);
    if (!my_config_manager.isKey("rgb_switch"))
        my_config_manager.putUChar("rgb_switch", rgb_switch);
    if (!my_config_manager.isKey("my_rgb_mode"))
        my_config_manager.putUChar("my_rgb_mode", my_rgb_mode);
    if (!my_config_manager.isKey("my_rgb_bn"))
        my_config_manager.putUChar("my_rgb_bn", my_rgb_bn);
    if (!my_config_manager.isKey("use_ex_keys"))
        my_config_manager.putUChar("use_ex_keys", use_ex_keys);
    if (!my_config_manager.isKey("screen_blk"))
        my_config_manager.putUChar("screen_blk", screen_blk);
    // 添加配置项目
}

void get_configs()
{
    connect_mode = my_config_manager.getUChar("connect_mode");
    rgb_switch = my_config_manager.getUChar("rgb_switch");
#ifdef USE_ONBOARD_RGB_CONFIGS
    my_rgb_mode = my_config_manager.getUChar("my_rgb_mode");
    my_rgb_bn = my_config_manager.getUChar("my_rgb_bn");
#endif
    use_ex_keys = my_config_manager.getUChar("use_ex_keys");
    screen_blk = my_config_manager.getUChar("screen_blk");
    // 在开机时获取配置项
    my_config_manager.end();
}

void init_configs()
{
    if (my_config_manager.begin(default_config_name))
    {
        // Serial.println();
        // Serial.print(default_config_name);
        // Serial.print("：可用存储空间：");
        // Serial.println(my_config_manager.freeEntries());
        create_configs();
        get_configs();
    }
}