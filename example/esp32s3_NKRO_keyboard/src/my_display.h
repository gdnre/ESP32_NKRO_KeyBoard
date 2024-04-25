#include "Arduino_GFX_Library.h"

extern Arduino_DataBus *bus;
extern Arduino_GFX *gfx;

extern bool screen_change;
extern RTC_DATA_ATTR uint8_t screen_blk;

void init_screen();
void screen_loop();
void screen_loop_task(void *pvParameters);