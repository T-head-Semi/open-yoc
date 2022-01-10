#ifndef __MONKEY_KEY_EVENT_H_
#define __MONKEY_KEY_EVENT_H_

#include "MonkeyEvent.h"
enum HW_KEYS {
   VOLUME_DOWN,VOLUME_UP,MUTE,
};

class MonkeyKeyEvent : public MonkeyEvent{
public:
    MonkeyKeyEvent();
    ~MonkeyKeyEvent();

    int injectEvent(int randomValue) override;

private:
    int min=0;
    int max=108;
};
#endif /* MonkeyKeyEvent_hpp */
