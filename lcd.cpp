#include "lcd.h"
#include <string>

void init_display() {
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(LTDC_ACTIVE_LAYER, LCD_FB_START_ADDRESS);
    BSP_LCD_SelectLayer(LTDC_ACTIVE_LAYER);
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetFont(&LCD_DEFAULT_FONT);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
}

void color_segment(int segment, uint32_t color) {
    BSP_LCD_SetTextColor(color);
    int x_size = BSP_LCD_GetXSize();
    int y_size = BSP_LCD_GetYSize();
    int x, y, height, width;
    height = y_size / 2;
    width = x_size / 2;
    x = (segment % 2 == 0) ? 0 : width;
    y = (segment < 2 ) ? 0 : height;
    BSP_LCD_FillRect(x, y, width, height);
}


void update_timer(int time) {
    BSP_LCD_SetTextColor(LCD_COLOR_RED);
    char time_str[6];
    sprintf(time_str, "%2d:%2d", (time / 60), (time % 60));
    BSP_LCD_DisplayStringAt(0, 0, (uint8_t *)time_str, LEFT_MODE);
}

void explode() {
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_RED);
}

void clear_display() {
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
}
