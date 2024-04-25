#include "my_rgb.h"

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_DIN, NEO_GRB + NEO_KHZ800);
RTC_DATA_ATTR uint8_t my_rgb_bn = 20;//255max
RTC_DATA_ATTR uint8_t my_rgb_mode = FX_MODE_STATIC;
RTC_DATA_ATTR bool rgb_switch = true;
bool rgb_change = false;

void init_my_rgb()
{ // 初始化rgb，先不读取配置，用来指示开机状态
    pinMode(LED_POWER, OUTPUT);
    digitalWrite(LED_POWER, LOW);
    ws2812fx.init();
    ws2812fx.setBrightness(64);
    ws2812fx.setSpeed(3000);
    ws2812fx.setMode(FX_MODE_RAINBOW_CYCLE);
    ws2812fx.setColor(PINK);
    ws2812fx.start();
    ws2812fx.service();
}

void rgb_loop()
{ // rgb状态切换以及rgb开启时数据发送
    if (rgb_change)
    {
        // my_rgb_mode += 1;
        // my_rgb_mode %= FX_MODE_OSCILLATOR;
        // ws2812fx.setMode(my_rgb_mode);
        // Serial.printf("当前rgb模式为：%d \n",my_rgb_mode);
        if (rgb_switch)
        { // 当前是打开状态，关闭
            ws2812fx.stop();
            pinMode(LED_POWER, INPUT);
        }
        else
        { // 当前是关闭状态，打开
            ws2812fx.start();
            pinMode(LED_POWER, OUTPUT);
            digitalWrite(LED_POWER, LOW);
        }

        rgb_switch = (!rgb_switch);

        my_config_manager.begin(default_config_name);
        my_config_manager.putUChar("rgb_switch", rgb_switch);
        my_config_manager.end();
        Serial.println("更改rgb配置成功");

        rgb_change = 0;

    }
    if (rgb_switch)
    {
        ws2812fx.service();
    }
}

void set_rgb_after_start()
{ // usb初始化完成后，根据配置内容设置rgb
    ws2812fx.setBrightness(my_rgb_bn);
    ws2812fx.setMode(my_rgb_mode);
    if (!rgb_switch)
    {
        ws2812fx.stop();
        pinMode(LED_POWER, INPUT);
    }
}

void rgb_loop_task(void *pvParameters)
{
    char *taskname = pcTaskGetName(NULL);
    int16_t freestack = uxTaskGetStackHighWaterMark(NULL);
    Serial.printf("任务<%s>开始运行，剩余堆栈为%d\n", taskname, freestack);

    init_my_rgb();
    vTaskDelay(1000/portTICK_PERIOD_MS);
    set_rgb_after_start();
    for (;;)
    {
        rgb_loop();
        vTaskDelay(2/portTICK_PERIOD_MS);
    }
}