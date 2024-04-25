#include <WS2812FX.h>
#include "my_esp32s3_configure.h"

extern WS2812FX ws2812fx;
extern RTC_DATA_ATTR uint8_t my_rgb_bn;
extern RTC_DATA_ATTR uint8_t my_rgb_mode;
extern RTC_DATA_ATTR bool rgb_switch;
extern bool rgb_change;

void init_my_rgb();
void rgb_loop();
void set_rgb_after_start();
void rgb_loop_task(void *pvParameters);