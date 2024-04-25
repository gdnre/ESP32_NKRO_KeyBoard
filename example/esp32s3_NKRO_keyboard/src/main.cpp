// 想要低点功耗就修改cpu频率为80Mhz，默认是240Mhz，现在负载下中间值和240差距不大，不用考虑，platformio.ini里的f_cpu的24xxx改成8xxx；
// 整板功耗测试：usb连接，手动复位开机，不开灯；
// 40Mhz   rgb显示异常
// 80Mhz   40ma
// 120Mhz  60ma
// 240Mhz  64ma
//蓝牙模式约55-60ma功耗，
//rgb功耗和亮度颜色有关，基本至少50ma，正常应该要按100-200ma算；
//我现在设定的粉色光，亮度20时功耗20ma，亮度255时功耗130ma

// #include <Arduino.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/timers.h"
// #include "freertos/event_groups.h"
// #include "sdkconfig.h"
// #include "esp_system.h"
// #include "soc/rtc_cntl_reg.h"
// #include "soc/rtc_io_reg.h"
// #include "soc/uart_reg.h"
// #include "soc/io_mux_reg.h"
// #include "soc/timer_group_reg.h"
// #include "rom/rtc.h"
// #include "rom/ets_sys.h"
// #include "rom/gpio.h"
#include "my_esp32s3_configure.h"
#include "my_keyboard.h"
#include "my_display.h"
#include "my_rgb.h"

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR uint32_t stub_high_level_pins;
RTC_DATA_ATTR uint32_t stud_scan_result[5];

unsigned long no_operate_timer = 0;
const bool enable_deep_sleep = true;        // 是否允许进入深睡,深睡时约等于关机，任意按键唤醒，不支持旋转编码器唤醒，usb连接会断开，所以不要设置太小
unsigned long deep_sleep_time = 20 * 60000; // 20分钟不操作进深睡,如果是usb连接，则该值×3

void setup()
{
  Serial.begin(115200);
  Set_Pins_To_Input(used_pins, pins_num);
  if (bootCount > 0)
  {
    // 从深睡醒来
    Serial.printf("===============================\nesp32s3深睡唤醒\n");
  }
  else
  { // 手动复位开机，需要获取板载配置
    init_configs();
    delay(500); // 延迟一会方便刷机
    Serial.printf("===============================\nesp32s3开机\n开机时间:");
    Serial.println(millis());
  }
  if (bootCount < 0)
  { // 溢出处理
    bootCount = 1;
  }

  start_keyboard();
  Serial.printf("空闲内部堆大小：%d\n", esp_get_free_internal_heap_size());
  Serial.printf("空闲外部堆大小：%d\n", esp_get_free_heap_size()); // 查看psram是否初始化成功

  xTaskCreatePinnedToCore(key_matrix_scan_task, "矩阵扫描任务", 4096, NULL /*参数*/, 2 /*优先级*/, NULL /*任务句柄*/, 1);
  xTaskCreatePinnedToCore(rgb_loop_task, "rgb循环任务", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(encoder_scan_task, "编码扫描任务", 4096, NULL /*参数*/, 2, NULL /*任务句柄*/, 1);
  xTaskCreatePinnedToCore(screen_loop_task, "屏幕循环任务", 4096, NULL /*参数*/, 1, NULL /*任务句柄*/, 1);
  xTaskCreatePinnedToCore(deep_sleep_check_task, "深睡任务", 2048, NULL /*参数*/, 1, NULL /*任务句柄*/, 0);
}

void loop() {}

void deep_sleep_check_task(void *pvParameters)
{
  char *taskname = pcTaskGetName(NULL);
  int16_t freestack = uxTaskGetStackHighWaterMark(NULL);
  Serial.printf("任务<%s>开始运行，剩余堆栈为%d\n", taskname, freestack);

  esp_deep_sleep_disable_rom_logging();

  for (;;)
  {
    vTaskDelay(deep_sleep_time / 10 / portTICK_PERIOD_MS);
    if (enable_deep_sleep && ((!connect_mode) && (millis() - no_operate_timer > deep_sleep_time)) || (connect_mode && (millis() - no_operate_timer > deep_sleep_time * 3)))
    {
      Serial.printf("即将进入睡眠模式，当前睡眠次数：%d\n", bootCount);
      digitalWrite(EX_KEY0, LOW);
      digitalWrite(EX_KEY1, LOW);
      if (use_ex_keys)
      {
        esp_sleep_enable_ext1_wakeup(WAKEUP_BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
      }
      else
      {
        esp_sleep_enable_ext1_wakeup(ROWPINS_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
      }
      esp_deep_sleep_start();
    }
    // vTaskDelay(300000 / portTICK_PERIOD_MS);
  }
}

void RTC_IRAM_ATTR esp_wake_deep_sleep()
{ // Wake Stub 从深睡中唤醒时立刻加载的函数，在boot之前调用
  bootCount++;
#define GETWAKEUPKEY 1
#ifdef GETWAKEUPKEY
  stub_high_level_pins = gpio_input_get() & (EXKEYS_BITMASK | ROWPINS_BITMASK);

  if (stub_high_level_pins & EXKEYS_BITMASK)
  {
    esp_default_wake_deep_sleep();
    return;
  }
  else if (stub_high_level_pins & ROWPINS_BITMASK)
  {
    // SetScanOutputHigh(COL0);
    // gpio_output_set_high(0x8,0x8,0x8,0x8);
    // 设置输出引脚为简单输出模式(不接入任何内部外设)
    REG_SET_FIELD(GPIO_FUNC39_OUT_SEL_CFG_REG, GPIO_FUNC39_OUT_SEL, 256);
    REG_SET_FIELD(GPIO_FUNC40_OUT_SEL_CFG_REG, GPIO_FUNC40_OUT_SEL, 256);
    REG_SET_FIELD(GPIO_FUNC41_OUT_SEL_CFG_REG, GPIO_FUNC41_OUT_SEL, 256);
    REG_SET_FIELD(GPIO_FUNC42_OUT_SEL_CFG_REG, GPIO_FUNC42_OUT_SEL, 256);
    REG_SET_FIELD(GPIO_FUNC38_OUT_SEL_CFG_REG, GPIO_FUNC38_OUT_SEL, 256);

    // 配置引脚为gpio
    REG_SET_FIELD(IO_MUX_GPIO39_REG, MCU_SEL, 1);
    REG_SET_FIELD(IO_MUX_GPIO40_REG, MCU_SEL, 1);
    REG_SET_FIELD(IO_MUX_GPIO41_REG, MCU_SEL, 1);
    REG_SET_FIELD(IO_MUX_GPIO42_REG, MCU_SEL, 1);
    REG_SET_FIELD(IO_MUX_GPIO38_REG, MCU_SEL, 1);

    // 设置输出引脚为开漏输出
    REG_SET_FIELD(GPIO_PIN39_REG, GPIO_PIN39_PAD_DRIVER, 1);
    REG_SET_FIELD(GPIO_PIN40_REG, GPIO_PIN40_PAD_DRIVER, 1);
    REG_SET_FIELD(GPIO_PIN41_REG, GPIO_PIN41_PAD_DRIVER, 1);
    REG_SET_FIELD(GPIO_PIN42_REG, GPIO_PIN42_PAD_DRIVER, 1);
    REG_SET_FIELD(GPIO_PIN38_REG, GPIO_PIN38_PAD_DRIVER, 1);

    // 配置引脚的开漏下拉使能
    REG_SET_FIELD(IO_MUX_GPIO39_REG, FUN_PD, 1);
    REG_SET_FIELD(IO_MUX_GPIO40_REG, FUN_PD, 1);
    REG_SET_FIELD(IO_MUX_GPIO41_REG, FUN_PD, 1);
    REG_SET_FIELD(IO_MUX_GPIO42_REG, FUN_PD, 1);
    REG_SET_FIELD(IO_MUX_GPIO38_REG, FUN_PD, 1);

    // 设置高位io输出寄存器不输出
    REG_SET_FIELD(GPIO_OUT1_W1TC_REG, GPIO_OUT1_W1TC, COLPINS_BITMASK); // 所有列引脚不输出
    // 0
    // 设置高位io输出寄存器为输出高电平
    REG_SET_FIELD(GPIO_OUT1_W1TS_REG, GPIO_OUT1_W1TS, HIGHPIN_BITMASK(COL0)); // col0上拉
    REG_SET_FIELD(GPIO_ENABLE1_W1TS_REG, GPIO_ENABLE1_W1TS, COLPINS_BITMASK); // 设置高位io输出使能寄存器使能
    // ets_delay_us(20);
    if (gpio_input_get() & ROWPINS_BITMASK)
    {
      stud_scan_result[0] = gpio_input_get() & ROWPINS_BITMASK;
    }

    // 1
    REG_SET_FIELD(GPIO_OUT1_W1TC_REG, GPIO_OUT1_W1TC, COLPINS_BITMASK);       // 所有列引脚不输出
    REG_SET_FIELD(GPIO_OUT1_W1TS_REG, GPIO_OUT1_W1TS, HIGHPIN_BITMASK(COL1)); // col1上拉
    // ets_delay_us(20);

    if (gpio_input_get() & ROWPINS_BITMASK)
    {
      stud_scan_result[1] = gpio_input_get() & ROWPINS_BITMASK;
    }

    // 2
    REG_SET_FIELD(GPIO_OUT1_W1TC_REG, GPIO_OUT1_W1TC, COLPINS_BITMASK);       // 所有列引脚不输出
    REG_SET_FIELD(GPIO_OUT1_W1TS_REG, GPIO_OUT1_W1TS, HIGHPIN_BITMASK(COL2)); // col1上拉
    // ets_delay_us(20);

    if (gpio_input_get() & ROWPINS_BITMASK)
    {
      stud_scan_result[2] = gpio_input_get() & ROWPINS_BITMASK;
    }

    // 3
    REG_SET_FIELD(GPIO_OUT1_W1TC_REG, GPIO_OUT1_W1TC, COLPINS_BITMASK);       // 所有列引脚不输出
    REG_SET_FIELD(GPIO_OUT1_W1TS_REG, GPIO_OUT1_W1TS, HIGHPIN_BITMASK(COL3)); // col1上拉
    // ets_delay_us(20);

    if (gpio_input_get() & ROWPINS_BITMASK)
    {
      stud_scan_result[3] = gpio_input_get() & ROWPINS_BITMASK;
    }

    // 4
    REG_SET_FIELD(GPIO_OUT1_W1TC_REG, GPIO_OUT1_W1TC, COLPINS_BITMASK);       // 所有列引脚不输出
    REG_SET_FIELD(GPIO_OUT1_W1TS_REG, GPIO_OUT1_W1TS, HIGHPIN_BITMASK(COL4)); // col1上拉
    // ets_delay_us(20);

    if (gpio_input_get() & ROWPINS_BITMASK)
    {
      stud_scan_result[4] = gpio_input_get() & ROWPINS_BITMASK;
    }

    // over

    REG_SET_FIELD(GPIO_ENABLE1_W1TC_REG, GPIO_ENABLE1_W1TC, COLPINS_BITMASK); // 设置高位io输出使能寄存器失能 高阻态maybe
  }
#endif
  esp_default_wake_deep_sleep(); // 必须有这个

  // pin_read = GPIO_INPUT_GET(15);
  // io0_io35 = REG_GET_FIELD(GPIO_IN_REG, GPIO_IN_DATA);

  // GPIO_IN_REG//gpio0-31焊盘对应的电平
  // IO_MUX_GPIO11_REG//io对应的输入寄存器,9号位/第10位为使能输入

  // GPIO_FUNC39_OUT_SEL //io对应的输出寄存器,前8位设置为256时结合之后的寄存器直接使能输出
  // GPIO_OUT1_REG GPIO_ENABLE1_REG//对应32-48号io的输出电平和使能
  // GPIO_OUT1_W1TC//用来清零out1
  // GPIO_OUT1_W1TS //write 1 to set,用来置位out1
  // GPIO_ENABLE1_W1TC_REG
}
// //方法备忘录////////////////////////////////////////////////
// const esp_pm_config_esp32s3_t power_manage = {
//     .max_freq_mhz = 240,
//     .min_freq_mhz = 80,
//     .light_sleep_enable = true,
// };
// esp_pm_configure(&power_manage);
// esp_sleep_pd_config();
// ///////////////////////////////////////////////////////////