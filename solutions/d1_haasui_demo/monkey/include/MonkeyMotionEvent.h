#ifndef __MONKEY_MOTION_EVENT_H_
#define __MONKEY_MOTION_EVENT_H_

#include "MonkeyEvent.h"

class MonkeyMotionEvent : public MonkeyEvent{
public:
    MonkeyMotionEvent();
    ~MonkeyMotionEvent();

    int injectEvent(int randomValue) override;

private:
    int height=480;
    int width=640;
};
#endif /* MonkeyMotionEvent_hpp */
