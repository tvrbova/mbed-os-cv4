#include "mbed.h"
#include "lcd.h"
#include <algorithm>

#define BOMB_TIME 60
#define EXPLODE_TIME 1000ms
Ticker timer_ticker;

#define FLAG_DEFAULT (1UL << 0)
#define FLAG_RESET (1UL << 1)
#define FLAG_EXPLODE (1UL << 2)
#define FLAG_STOP (1UL << 3)

char tlac[3] = {'r', 's', 'e'};
uint32_t colors[3] = {LCD_COLOR_RED, LCD_COLOR_GREEN, LCD_COLOR_YELLOW};
EventFlags event_flags;

Mutex lcd_mutex, flags_mutex;
Watchdog &watchdog = Watchdog::get_instance();

Thread th_explode, th_reset, th_stop;
// jedno tlacitko watchdog kick, jinek stop, jine spusti sekvemci exploze a kick
Thread th_timer, th_touch;

//https://os.mbed.com/teams/ST/code/DISCO-F746NG_LCDTS_demo//file/9f66aabe7b3b/main.cpp/ detekovani dotyku displaye

void update_display()
{
    lcd_mutex.lock();
    update_timer(watchdog.get_timeout());
    lcd_mutex.unlock();
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
    random_shuffle(&tlac[0], &tlac[3]);
    random_shuffle(&colors[0], &colors[3]);
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
        watchdog.stop();

        set_default_flag();
    }
}

void init() {
    lcd_mutex.lock();
    init_display();
    lcd_mutex.unlock();

    flags_mutex.lock();
    event_flags.set(FLAG_DEFAULT);
    flags_mutex.unlock();

    watchdog.start(BOMB_TIME);
    reset_display();
}



int main()
{
    init();

    th_timer.start(start_display_timer);
    th_touch.start(start_touch_detection);
    th_explode.start(wait_for_explode);
    th_reset.start(wait_for_reset);
    th_stop.start(wait_for_stop);

    while (true) {

    }
}

