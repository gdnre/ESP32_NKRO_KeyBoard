#include "my_display.h"
#include "my_esp32s3_configure.h"

Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC /* DC */, LCD_CS /* CS */, LCD_SCL /* SCK */, LCD_SDA /* MOSI */, GFX_NOT_DEFINED /* MISO */, HSPI /* spi_num */);

Arduino_GFX *gfx = new Arduino_ST7789(
    bus, LCD_RES /* RST */, 3 /* rotation */, true /* IPS */,
    135 /* width */, 240 /* height */,
    52 /* col offset 1 */, 40 /* row offset 1 */,
    53 /* col offset 2 */, 40 /* row offset 2 */);

bool screen_change = false;
RTC_DATA_ATTR uint8_t screen_blk = 1;

void init_screen()
{
  if (!use_ex_keys)
  {
    gfx->begin();
    gfx->fillScreen(OLIVE);
    gfx->println("ESP32S3 KeyBoard!");
    pinMode(LCD_BL, OUTPUT);
    if (screen_blk > 1)
    {
      analogWrite(LCD_BL, screen_blk);
    }
    else
    {
      digitalWrite(LCD_BL, screen_blk);
    }
  }
  else
  {
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, LOW);
    pinMode(EX_KEY0, INPUT_PULLDOWN);
    pinMode(EX_KEY1, INPUT_PULLDOWN);
  }
}

// #define CHANGE_NOW 1 // 下次重启时会才会更改配置，取消注释后会马上更改，但串口会打印一段错误信息，不确定会不会有不良影响
void screen_loop()
{
  if (screen_change)
  {
    my_config_manager.begin(default_config_name);
#ifdef CHANGE_NOW
    use_ex_keys = (!use_ex_keys);
    my_config_manager.putUChar("use_ex_keys", use_ex_keys);
    Serial.printf("更改屏幕配置成功,扩展按键状态为%d\n", use_ex_keys);
#else
    my_config_manager.putUChar("use_ex_keys", !use_ex_keys);
    Serial.printf("更改屏幕配置成功,扩展按键状态改为%d,重启后生效\n", !use_ex_keys);
#endif
    my_config_manager.end();

#ifdef CHANGE_NOW
    init_screen();
#endif
    screen_change = false;
  }
  if (!use_ex_keys)
  {
    // 屏幕的数据
    gfx->fillScreen(WHITE);
    gfx->setTextColor(random());
    gfx->setCursor(random(0, 135), random(0, 240));
    gfx->println("ESP32S3 KeyBoard!");
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  else
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void screen_loop_task(void *pvParameters)
{
  char *taskname = pcTaskGetName(NULL);
  int16_t freestack = uxTaskGetStackHighWaterMark(NULL);
  Serial.printf("任务<%s>开始运行，剩余堆栈为%d\n", taskname, freestack);

  init_screen();
  for (;;)
  {
    screen_loop();
  }
}