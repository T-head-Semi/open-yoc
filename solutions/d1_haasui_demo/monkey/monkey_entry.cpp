#include <memory>
#include <iostream>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <aos/kernel.h>
#include <time.h>
#include "MonkeyMotionEvent.h"
#include "MonkeyKeyEvent.h"
#include "MonkeyEvent.h"
#include "monkey_entry.h"

using namespace std;

/* Mokey test parameters */
int mSeed         = 0;
int mKeyPct       = 0;
int mAppswitchPct = 0;
int mTouchPct = 0;
int mCount    = 0;
bool systemCrashed = true;

int runMonkeyCycles(){
    if (mSeed == 0) {
        mSeed = (int)clock();
    }
    srand(mSeed);

    int cycleCounter = 0;
    //TODO 根据OS判断
    //shared_ptr<InputDeviceLinux> inputDevice = make_shared<InputDeviceLinux>();
    shared_ptr<MonkeyKeyEvent> keyEv = make_shared<MonkeyKeyEvent>();
    shared_ptr<MonkeyMotionEvent> motionEv = make_shared<MonkeyMotionEvent>();
    while (!systemCrashed /*&& cycleCounter++ < mCount*/) {
        int cls = rand()%100;
        //这里的顺序不能随意调整，和adjustPctFactor的加权有关
        if(cls<=mKeyPct){
            keyEv->injectEvent(rand());
        }else if(cls<=mAppswitchPct){
            //printf("it is Appswitch event \n");
        }else { //touch事件兜底
            motionEv->injectEvent(rand());
        }
        aos_msleep(1000);
    }
    return cycleCounter;
}

void adjustPctFactor(){
    while(mTouchPct>100){
        mTouchPct /=10;
    }
    while(mKeyPct>100){
        mKeyPct /=10;
    }
    while(mAppswitchPct>100){
        mAppswitchPct /=10;
    }

    mAppswitchPct = mKeyPct + mAppswitchPct;
    mTouchPct = 100 - mAppswitchPct;
}

void run(void *arg) {
    (void)arg;
    //todo 根据当前os进行判断
    adjustPctFactor();
    systemCrashed = false;
    runMonkeyCycles();
}

int monkey_entry(int argc, char * argv[]) {
    aos_task_t monkey_handle;
    aos_task_new_ext(&monkey_handle, "monkey_test", run, NULL, 0x2800, 48);
    return 0;
}

#include <aos/cli.h>
ALIOS_CLI_CMD_REGISTER(monkey_entry, monkey, monkey test);