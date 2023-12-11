#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"

void init_display();
void color_segment(int segment, uint32_t color);
void update_timer(int time);
void explode();
void clear_display();