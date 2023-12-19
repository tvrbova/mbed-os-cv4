#include "mbed.h"
#include "lcd.h"
#include <time.h>

const uint32_t BOMB_TIME = 10000;
#define EXPLODE_TIME 1000ms
Ticker timer_ticker;

#define FLAG_DEFAULT (1UL << 0)
#define FLAG_RESET (1UL << 1)
#define FLAG_EXPLODE (1UL << 2)
#define FLAG_STOP (1UL << 3)

char tlac[3] = {'r', 's', 'e'};
char colors[3] = {'r', 'g', 'y'};
EventFlags event_flags;

Mutex lcd_mutex, flags_mutex;
Watchdog &watchdog = Watchdog::get_instance();

Thread th_explode, th_reset, th_stop;
Thread th_touch;

void update_display()
{
    flags_mutex.lock();
    update_timer((int)(watchdog.get_timeout()/1000));
    flags_mutex.unlock();
}

void start_touch_detection() {
    int x_size = BSP_LCD_GetXSize();
    int y_size = BSP_LCD_GetYSize();
    int height, width;
    height = y_size / 2;
    width = x_size / 2;
    char action = 'd';

    TS_StateTypeDef TS_State;
    uint16_t x, y;
    while (1) {
        BSP_TS_GetState(&TS_State);
        if (TS_State.touchDetected) {
            x = TS_State.touchX[0];
            y = TS_State.touchY[0];

            if (x > width) {
                if (y > height) {
                    action = tlac[2];
                } else {
                    action = tlac[0];
                }
            } else {
                if (y > height) {
                    action = tlac[1];
                }
            }

            flags_mutex.lock();
            switch (action) {
                case 'r':
                    event_flags.set(FLAG_RESET);
                    break;
                case 's':
                    event_flags.set(FLAG_STOP);
                    break;
                case 'e':
                    event_flags.set(FLAG_EXPLODE);
                    break;
                default:
                    event_flags.set(FLAG_DEFAULT);
                    break;
            }
            flags_mutex.unlock();

            action = 'd';
        }
    }
}

void shuffle_char(char *array, size_t n)
{
    if (n > 1) 
    {
        size_t i;
        for (i = 0; i < n - 1; i++) 
        {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          char t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}

void reset_display() {
    shuffle_char(tlac, 3);
    shuffle_char(colors, 3);
    for (int i = 0; i < 3; i++) {
        lcd_mutex.lock();
        uint32_t color;
        switch (colors[i]) {
            case 'r':
                color = LCD_COLOR_RED; break;
            case 'g':
                color = LCD_COLOR_GREEN; break;
            case 'y':
                color = LCD_COLOR_YELLOW; break;
            default:
                color = LCD_COLOR_BLUE; break;
        }
        color_segment((i+1), color);
        lcd_mutex.unlock();
    }
}

void set_default_flag() {
    flags_mutex.lock();
    event_flags.set(FLAG_DEFAULT);
    flags_mutex.unlock();
}

void wait_for_explode() {
    uint32_t flags_read = 0;
    while (1) {
        flags_read = event_flags.wait_any(FLAG_EXPLODE);

        timer_ticker.detach();
        lcd_mutex.lock();
        explode();
        lcd_mutex.unlock();

        ThisThread::sleep_for(EXPLODE_TIME);

        lcd_mutex.lock();
        clear_display();
        lcd_mutex.unlock();
        timer_ticker.attach(&update_display, 10ms);

        watchdog.kick();
        reset_display();

        set_default_flag();
    }
}

void wait_for_reset() {
    uint32_t flags_read = 0;
    while (1) {
        flags_read = event_flags.wait_any(FLAG_RESET);
        watchdog.kick();
        set_default_flag();
    }
}

void wait_for_stop() {
    uint32_t flags_read = 0;
    while (1) {
        flags_read = event_flags.wait_any(FLAG_STOP);

        while (true) {
            watchdog.kick();
        }
        
        set_default_flag();
    }
}

void init() {
    srand(time(NULL));
    lcd_mutex.lock();
    init_display();
    lcd_mutex.unlock();

    set_default_flag();

    reset_display();
    
    update_timer(10);
    watchdog.start(BOMB_TIME);
}



int main()
{
    init();
    timer_ticker.attach(&update_display, 10ms);
    th_touch.start(start_touch_detection);
    th_explode.start(wait_for_explode);
    th_reset.start(wait_for_reset);
    th_stop.start(wait_for_stop);

    while (true) {
    }
}

