#ifndef __MONKEY_EVENT_H_
#define __MONKEY_EVENT_H_
#include <unistd.h>
#include <stdio.h>
#include <memory>
extern "C"{
   #include "InputDevice.h"
}


using namespace std;

class MonkeyEvent
{

public:
//    MonkeyEvent();
//    ~MonkeyEvent();

//    int getEventType() {
//        return mEventType;
//    }

   virtual int injectEvent(int randomValue)=0;

public:
//    int mEventType;
};


#endif /* MonkeyEvent_h */
