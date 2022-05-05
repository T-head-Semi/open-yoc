#ifndef _TIMER_PORT_H_
#define _TIMER_PORT_H_
#include "aos/kernel.h"
#include "bluetooth/bluetooth.h"
#include "ble_os_port.h"

typedef union {
#if (RHINO_CONFIG_TIMER > 0)
    aos_timer_t aos_timer;
#endif
    k_timer_t  k_timer;
} timer_s;


typedef struct {
    timer_s timer;
} ota_timer_t;

typedef void (*ota_timer_handler_t)(void *timer, void *args);

int ota_timer_init(ota_timer_t *timer, ota_timer_handler_t handle, void *args);
int ota_timer_start(ota_timer_t *timer, uint32_t timeout);
int ota_timer_stop(ota_timer_t *timer);
void ota_timer_free(ota_timer_t *timer);





#endif
