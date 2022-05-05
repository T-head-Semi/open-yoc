#ifndef __MESH_SAL_LPM_H__
#define __MESH_SAL_LPM_H__

#include <stdint.h>

typedef void (*mesh_io_wakeup_cb)(void* arg);

enum io_pol {
    FALLING = 0,
    RISING  = 1,
    ACT_LOW  = 2,
    ACT_HIGH = 3,
};

#ifndef bool
#define bool unsigned char
#endif

void mesh_sal_sleep_enable();
void mesh_sal_sleep_disable();
int  mesh_sal_sleep_enter_standby();
int  mesh_sal_sleep_wakup_io_set (uint8_t               port, uint8_t pol);
int mesh_sal_sleep_wakeup_io_get_status (uint8_t port);
int mesh_sal_io_wakeup_cb_register (mesh_io_wakeup_cb cb);
int mesh_sal_io_wakeup_cb_unregister();

#endif
