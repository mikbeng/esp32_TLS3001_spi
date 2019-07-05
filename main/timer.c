
#include "driver/timer.h"
#include "timer.h"
#include "main.h"
#include "driver/gpio.h"

static intr_handle_t s_timer_handle;
SemaphoreHandle_t xSemaphore_delay = NULL;

static void timer_isr(void* arg)
{
    
    static BaseType_t xHigherPriorityTaskWoken;

    timer_pause(TIMER_GROUP_0, TIMER_0);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);

    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[0].config.alarm_en = 1;

    xHigherPriorityTaskWoken = pdFALSE;
    // Unblock the task by releasing the semaphore.
    xSemaphoreGiveFromISR( xSemaphore_delay, &xHigherPriorityTaskWoken );

    // your code, runs in the interrupt
}

void init_timer()
{
    timer_config_t config = {
            .alarm_en = true,
            .counter_en = false,
            .intr_type = TIMER_INTR_LEVEL,
            .counter_dir = TIMER_COUNT_UP,
            .auto_reload = false,
            .divider = 80   // 1 us per tick 
    };
    
    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, &timer_isr, NULL, 0, &s_timer_handle);

    vSemaphoreCreateBinary(xSemaphore_delay);
}

esp_err_t user_timer_start()
{
    timer_start(TIMER_GROUP_0, TIMER_0);
    return ESP_OK;
}

esp_err_t user_timer_set_delay(int timer_period_us)
{
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, timer_period_us);
    return ESP_OK;
}
