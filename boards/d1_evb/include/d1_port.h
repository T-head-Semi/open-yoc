#ifndef _PORT_H
#define _PORT_H

#ifdef __cplusplus    /* C++编译器包含的宏，例如用g++编译时，该宏就存在，则下面的语句extern "C"才会被执行 */
extern "C" {          /* C++编译器才能支持，C编译器不支持 */
#endif

#define HAASUI_WINDOW_WIDTH  (800)
#define HAASUI_WINDOW_HEIGHT (480)

#define HAASUI_SHOW_RED   (0)
#define HAASUI_RGB_TEST   (1)
#define HAASUI_HELLO_DEMO (2)
#define HAASUI_UED_DEMO   (3)
#define HAASUI_DEMO_MAX   (HAASUI_UED_DEMO)

void display_draw(void *buffer);

void ui_task_run(void);

#ifdef __cplusplus
}
#endif

#endif //_PORT_H
