#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <drv/display.h>
#include <drv/g2d.h>
#include <drv/porting.h>
#include <aos/hal/touch.h>
#include "d1_port.h"
#include "uinput.h"

#define SCREEN_WITH    800
#define SCREEN_HEIGHT  1280

#define TOUCH_MAX_X    SCREEN_HEIGHT
#define TOUCH_MAX_Y    SCREEN_WITH

event_notify_cb touch_notify = NULL;
static int revert_x_flag = 1;
static int revert_y_flag = 1;
static int exchange_x_y_flag = 1;

void display_fb_v2h(void *src_buf, int src_w, int src_h, void *dst_buf)
{
    int ret = 0;
    g2d_blt_h blit_para;
    static uint8_t screen_buf[HAASUI_WINDOW_WIDTH * HAASUI_WINDOW_HEIGHT * 4];

    ret = csi_g2d_open();
    if (ret) {
        printf("g2d open fail\n");
        return;
    }

    soc_dcache_clean_range((unsigned long)src_buf, HAASUI_WINDOW_WIDTH*HAASUI_WINDOW_HEIGHT*4);

    /* Step 1. Rotate image */
    memset(&blit_para, 0, sizeof(blit_para));

    blit_para.src_image_h.laddr[0] = (uint32_t)((unsigned long)src_buf);
    // blit_para.src_image_h.laddr[0] = (int)(blit_para.src_image_h.laddr[0] );
    blit_para.src_image_h.use_phy_addr = 1;
    blit_para.src_image_h.format = G2D_FORMAT_ARGB8888;
    blit_para.src_image_h.mode = G2D_GLOBAL_ALPHA;
    blit_para.src_image_h.color = 0xee8899;
    blit_para.src_image_h.alpha = 0xff;
    blit_para.src_image_h.width = HAASUI_WINDOW_WIDTH;
    blit_para.src_image_h.height = HAASUI_WINDOW_HEIGHT;
    blit_para.src_image_h.clip_rect.x = 0;
    blit_para.src_image_h.clip_rect.y = 0;
    blit_para.src_image_h.clip_rect.w = HAASUI_WINDOW_WIDTH;
    blit_para.src_image_h.clip_rect.h = HAASUI_WINDOW_HEIGHT;

    blit_para.dst_image_h.laddr[0] = (uint32_t)((unsigned long)screen_buf);
    // blit_para.dst_image_h.laddr[0] = (int)(blit_para.dst_image_h.laddr[0] );
    blit_para.dst_image_h.use_phy_addr = 1;
    blit_para.dst_image_h.format = G2D_FORMAT_ARGB8888;
    blit_para.dst_image_h.mode = G2D_GLOBAL_ALPHA;
    blit_para.dst_image_h.color = 0xee8899;
    blit_para.dst_image_h.alpha = 0xff;
    blit_para.dst_image_h.width = HAASUI_WINDOW_HEIGHT;
    blit_para.dst_image_h.height = HAASUI_WINDOW_WIDTH;
    blit_para.dst_image_h.clip_rect.x = 0;
    blit_para.dst_image_h.clip_rect.y = 0;
    blit_para.dst_image_h.clip_rect.w = HAASUI_WINDOW_HEIGHT;
    blit_para.dst_image_h.clip_rect.h = HAASUI_WINDOW_WIDTH;

    blit_para.flag_h = G2D_ROT_90;

    if(csi_g2d_ioctl(G2D_CMD_BITBLT_H ,(&blit_para)) < 0)
    {
        printf("[%d][%s][%s]G2D_CMD_BITBLT_H failure!\n",__LINE__, __FILE__,__FUNCTION__);
        csi_g2d_close();
        return;
    }

    /* Step 2. Enlarge image */
    memset(&blit_para, 0, sizeof(blit_para));

    blit_para.src_image_h.laddr[0] = (uint32_t)((unsigned long)screen_buf);
    // blit_para.src_image_h.laddr[0] = (int)(blit_para.src_image_h.laddr[0] );
    blit_para.src_image_h.use_phy_addr = 1;
    blit_para.src_image_h.format = G2D_FORMAT_ARGB8888;
    blit_para.src_image_h.mode = G2D_PIXEL_ALPHA;
    blit_para.src_image_h.color = 0xee8899;
    blit_para.src_image_h.alpha = 0xff;
    blit_para.src_image_h.width = HAASUI_WINDOW_HEIGHT;
    blit_para.src_image_h.height = HAASUI_WINDOW_WIDTH;
    blit_para.src_image_h.clip_rect.x = 0;
    blit_para.src_image_h.clip_rect.y = 0;
    blit_para.src_image_h.clip_rect.w = HAASUI_WINDOW_HEIGHT;
    blit_para.src_image_h.clip_rect.h = HAASUI_WINDOW_WIDTH;

    blit_para.dst_image_h.laddr[0] = (uint32_t)((unsigned long)dst_buf);
    // blit_para.dst_image_h.laddr[0] = (int)(blit_para.dst_image_h.laddr[0] );
    blit_para.dst_image_h.use_phy_addr = 1;
    blit_para.dst_image_h.format = G2D_FORMAT_ARGB8888;
    blit_para.dst_image_h.mode = G2D_PIXEL_ALPHA;
    blit_para.dst_image_h.color = 0xee8899;
    blit_para.dst_image_h.alpha = 0xff;
    blit_para.dst_image_h.width = SCREEN_WITH;
    blit_para.dst_image_h.height = SCREEN_HEIGHT;
    blit_para.dst_image_h.clip_rect.x = 0;
    blit_para.dst_image_h.clip_rect.y = 0;
    blit_para.dst_image_h.clip_rect.w = SCREEN_WITH;
    blit_para.dst_image_h.clip_rect.h = SCREEN_HEIGHT;

    blit_para.flag_h = G2D_BLT_NONE_H;

    if(csi_g2d_ioctl(G2D_CMD_BITBLT_H, (&blit_para)) < 0)
    {
        printf("[%d][%s][%s]G2D_CMD_BITBLT_H failure!\n",__LINE__, __FILE__,__FUNCTION__);
        csi_g2d_close();
        return;
    }

    csi_g2d_close();
}

void display_draw(void *buffer)
{
    int ret;
    uint64_t arg[3];
    disp_layer_config_t config;
    static uint8_t framebuf[SCREEN_WITH * SCREEN_HEIGHT * 4];

    /* Here, we redirect picture display from horizantal to vertical by G2D, so use SCREEN_HEIGHT as dst with, SCREEN_WITH as height */
    display_fb_v2h(buffer, HAASUI_WINDOW_WIDTH, HAASUI_WINDOW_HEIGHT, framebuf);

    memset(&config, 0, sizeof(disp_layer_config_t));
    config.channel = 0;
    config.layer_id = 0;
    config.enable = 1;
    config.info.mode = LAYER_MODE_BUFFER;
    config.info.fb.addr[0] = (unsigned long long)framebuf;
    config.info.fb.size[0].width = SCREEN_WITH;
    config.info.fb.size[0].height = SCREEN_HEIGHT;
    config.info.fb.align[0] = 4;
    config.info.fb.format = DISP_FORMAT_ARGB_8888;
    config.info.fb.crop.x = 0;
    config.info.fb.crop.y = 0;
    config.info.fb.crop.width = ((unsigned long)SCREEN_WITH) << 32;
    config.info.fb.crop.height = ((unsigned long)SCREEN_HEIGHT) << 32;
    config.info.fb.flags = DISP_BF_NORMAL;
    config.info.fb.scan = DISP_SCAN_PROGRESSIVE;
    config.info.alpha_mode = 1;
    config.info.alpha_value = 0xff;
    config.info.screen_win.x = 0;
    config.info.screen_win.y = 0;
    config.info.screen_win.width = SCREEN_WITH;
    config.info.screen_win.height = SCREEN_HEIGHT;
    config.info.id = 0;
    arg[0] = 0; // screen0
    arg[1] = (unsigned long)&config;
    arg[2] = 1; // one layer
    ret = csi_display_ioctl(DISP_LAYER_SET_CONFIG, (void *)arg);
    if (ret != 0) {
        printf("dispay error.\n");
    }
}

static void __touchscreen_config(uint16_t *x, uint16_t *y)
{
    uint16_t _tmp;
    if (1 == exchange_x_y_flag) {
        _tmp = *x;
        *x = *y;
        *y = _tmp;
    }
    if (1 == revert_x_flag) {
        *x = TOUCH_MAX_X - *x;
    }
    if (1 == revert_y_flag) {
        *y = TOUCH_MAX_Y - *y;
    }
}

static void touchscreen_event(touch_message_t msg, void* arg)
{
    uint16_t x, y;
    uinput_event_t tevent;

    x = msg->x;
    y = msg->y;

    __touchscreen_config(&x, &y);

    /* Screen coordinates are converted to Haas UI View coordinates */
    x = x * HAASUI_WINDOW_WIDTH / TOUCH_MAX_X;
    y = y * HAASUI_WINDOW_HEIGHT / TOUCH_MAX_Y;

    switch (msg->event)
    {
    case TOUCH_EVENT_MOVE:
        //printk("move (%d, %d)\r\n", x, y);
        tevent.type = UINPUT_EVENT_TOUCH_MOTION;
        tevent.abs.x = x;
        tevent.abs.y = y;
        (*touch_notify)(tevent.type, &tevent, sizeof(uinput_event_t));
        break;

    case TOUCH_EVENT_DOWN:
        //printk("down (%d, %d)\r\n", x, y);
        tevent.abs.x = x;
        tevent.abs.y = y;
        tevent.type = UINPUT_EVENT_TOUCH_DOWN;
        if (touch_notify != NULL) {
            (*touch_notify)(tevent.type, &tevent, sizeof(uinput_event_t));
        }
        break;

    case TOUCH_EVENT_UP:
        //printk("up (%d, %d)\r\n", x, y);
        tevent.abs.x = x;
        tevent.abs.y = y;
        tevent.type = UINPUT_EVENT_TOUCH_UP;
        if (touch_notify != NULL) {
            (*touch_notify)(tevent.type, &tevent, sizeof(uinput_event_t));
        }
        break;

    default:
        break;
    }
}

int uinput_service_init(event_notify_cb func)
{
    touch_notify = func;
    hal_touch_cb_register(touchscreen_event, NULL);
    return 0;
}

#include <aos/kernel.h>
#ifndef CONFIG_UI_TASK_STACK_SIZE
#define CONFIG_UI_TASK_STACK_SIZE 65536
#endif
static void ui_task(void *arg)
{
    extern int falcon_entry(int argc, char *argv[]);
    printf("haasui entry here!\r\n");
    printf("haasui build time: %s, %s\r\n", __DATE__, __TIME__);
    hal_touch_init(NULL, NULL);
    falcon_entry(0, NULL);
}

void ui_task_run(void)
{
    aos_task_t task;
    aos_task_new_ext(&task, "ui-task", ui_task, NULL, CONFIG_UI_TASK_STACK_SIZE, AOS_DEFAULT_APP_PRI);
}