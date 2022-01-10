#include <stdbool.h>
#include <stdio.h>
#include <aos/kernel.h>
#include <aos/hal/touch.h>

#include "InputDevice.h"

void touchscreen_event(touch_message_t msg);

static aos_queue_t queue_handle;                                 /* queue handle */
static char        queue_buffer[sizeof(struct touch_message) * 10];   /* for the internal buffer of the queue */

static void touch_run(void* arg)
{
    touch_cb cb_func         = arg;
    struct touch_message msg = {0};
    size_t rev_size          = 0;

    while (1) {
        if (aos_queue_recv(&queue_handle, AOS_WAIT_FOREVER, &msg, &rev_size) == 0) {
            if (cb_func) {
                cb_func(&msg);
            }
        }
    }
}

void closeInputDevice(void)
{

}

int openInputDevice(void)
{
    static int init_flag = 0;
    aos_task_t task;
    aos_status_t status;

    if (init_flag == 1) {
        return -1;
    }

    status = aos_queue_new(&queue_handle, (void *)queue_buffer, sizeof(queue_buffer),
                           sizeof(struct touch_message));
    if (status != 0) {
        return -1;
    }

    status = aos_task_new_ext(&task, "simtouch", touch_run, touchscreen_event, 2048, 48);
    if (status != 0) {
        return -1;
    }

    init_flag = 1;

    return 0;
}

void getDisplaySize(int *height, int *width)
{
    *height = 1280;
    *width  = 800;
}

void getTGKeyValue(int random, int *code)
{
}

void getKeyScope(int *min, int *max)
{
    *min = 110;
    *max = 130;
}

int writeEventOnce(int fd, struct touch_message *event)
{
    aos_queue_send(&queue_handle, (void *)event, sizeof(struct touch_message));
    return 0;
}

/* key event */
int sendKeyEvent(int keycode)
{
    return 0;
}

/* One click, including Touch Down and Touch Up */
int sendTapEvent(int x, int y)
{
    struct touch_message event;

    event.x = x;
    event.y = y;
    event.event = TOUCH_EVENT_DOWN;
    writeEventOnce(0, &event);

    event.x = x;
    event.y = y;
    event.event = TOUCH_EVENT_UP;
    writeEventOnce(0, &event);
    return 0;
}

/* Touch screen movement event, including Touch Down, Move, and Touch Up */
int sendSwipeEvent(int x1, int y1, int x2, int y2, int duration)
{
    struct touch_message event;
    int step = 25;          //滑动步进，可调整
    int deltaX = (x2 - x1) / step;
    int deltaY = (y2 - y1) / step;

    //touch down
    aos_msleep(20);
    event.x = x1;
    event.y = y1;
    event.event = TOUCH_EVENT_DOWN;
    writeEventOnce(0, &event);

    //swipe
    for (int i = 0; i < step; i++)
    {
        x1 += deltaX;
        y1 += deltaY;

        event.x = x1;
        event.y = y1;
        event.event = TOUCH_EVENT_MOVE;
        writeEventOnce(0, &event);

        aos_msleep(duration / step);
    }
    //touch up
    event.x = x2;
    event.y = y2;
    event.event = TOUCH_EVENT_UP;
    writeEventOnce(0, &event);

    aos_msleep(20);
    return 0;
}

/* Return to the previous page */
int sendBackEvent()
{
    struct touch_message event;

    event.x = 760;
    event.y = 1240;
    event.event = TOUCH_EVENT_DOWN;
    writeEventOnce(0, &event);

    event.x = 760;
    event.y = 1240;
    event.event = TOUCH_EVENT_UP;
    writeEventOnce(0, &event);
    return 0;
}

