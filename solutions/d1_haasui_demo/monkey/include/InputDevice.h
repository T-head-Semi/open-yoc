#ifndef __INPUT_DEVICE_H_
#define __INPUT_DEVICE_H_

extern int openInputDevice(void);
extern void closeInputDevice(void);
extern void getDisplaySize(int* height, int* width);
extern void getKeyScope(int* min, int* max);
extern void getTGKeyValue(int random, int *code);

extern int sendKeyEvent(int keycode);
extern int sendTapEvent(int x, int y);
extern int sendSwipeEvent(int x1, int y1, int x2, int y2, int duration);
extern int sendSwitchAppEvent(char* appid);
int sendBackEvent();


#endif /* InputDevice_h */
