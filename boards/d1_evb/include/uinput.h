//
// Created by kevin.lwx on 2021/1/8.
//

#ifndef FALCON_UINPUT_H
#define FALCON_UINPUT_H

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup aos_uinput uinput
 *  This is the brief description of the component.
 *
 *  This is the detailed description, and it's optional.
 *  @{
 */

/* Here is Macro and struct definition*/
#define UINPUT_DEBUG_STEP printf("===excute to %d in %s===\n", __LINE__, __func__);

typedef enum {
    UINPUT_EVENT_NONE = 0,
    UINPUT_EVENT_KEY_DOWN,
    UINPUT_EVENT_KEY_UP,
    UINPUT_EVENT_TOUCH_DOWN,
    UINPUT_EVENT_TOUCH_UP,
    UINPUT_EVENT_TOUCH_MOTION,
    UINPUT_EVENT_TOUCH_CANCEL,
    UINPUT_EVENT_TOUCH_FRAME
} uinput_event_type_t;


typedef enum {
    UINPUT_SERVICE_KEYPAD,
    UINPUT_SERVICE_TOUCHPANEL,
    UINPUT_SERVICE_MT_TOUCHPANEL,
    UINPUT_MAX_CNT,
} uinput_type_e;

typedef struct {
    uinput_event_type_t type;
    union {
        int key_code;
        struct {
            int x;
            int y;
        } abs;
    };
} uinput_event_t;

typedef void (*event_notify_cb)(uinput_event_type_t event, void *pdata, uint32_t len);

/* Here is API and callback definition*/
/**
 * uinput initialization.
 *
 * @retval           0: fd; -1: initial failed.
 * @note
 */
int uinput_service_init(event_notify_cb func);
int uinput_open_devices(void);
#ifdef CONFIG_ENABLE_UINPUT_TEST
void uinput_test_input_event(void);
#endif
/** @} */

#ifdef __cplusplus
}
#endif

#endif  //FALCON_UINPUT_H
