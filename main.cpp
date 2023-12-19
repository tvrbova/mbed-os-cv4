#include "mbed.h"
#include "lcd.h"
#include <time.h>
//#include <algorithm>

const uint32_t BOMB_TIME = 10000;
#define EXPLODE_TIME 1000ms
Ticker timer_ticker;

#define FLAG_DEFAULT (1UL << 0)
#define FLAG_RESET (1UL << 1)
#define FLAG_EXPLODE (1UL << 2)
#define FLAG_STOP (1UL << 3)

char tlac[3] = {'r', 's', 'e'};
uint32_t colors[] = {LCD_COLOR_RED, LCD_COLOR_GREEN, LCD_COLOR_YELLOW};
EventFlags event_flags;

Mutex lcd_mutex, flags_mutex;
Watchdog &watchdog = Watchdog::get_instance();

Thread th_explode, th_reset, th_stop;
Thread th_touch;

int testcas = 10000;
DigitalOut led1(LED1);

//https://os.mbed.com/teams/ST/code/DISCO-F746NG_LCDTS_demo//file/9f66aabe7b3b/main.cpp/ detekovani dotyku displaye

void update_display()
{
    testcas --;
    //led1 = !led1;
    flags_mutex.lock();
    update_timer((int)(testcas/1000));
    flags_mutex.unlock();
}

void start_display_timer() {
    timer_ticker.attach(&update_display, 10ms);
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

void reset_display() {
    //random_shuffle(&tlac[0], &tlac[3]);
    //random_shuffle(&colors[0], &colors[3]);
    for (int i = 0; i < 3; i++) {
        lcd_mutex.lock();
        color_segment((i+1), colors[i]);
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
        start_display_timer();

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
    led1 = 0;
    init() ;
    watchdog.kick();

    start_display_timer();
    th_touch.start(start_touch_detection);
    th_explode.start(wait_for_explode);
    th_reset.start(wait_for_reset);
    th_stop.start(wait_for_stop);

    while (true) {

    }
}

