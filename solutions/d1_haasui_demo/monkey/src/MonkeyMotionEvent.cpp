#include "MonkeyMotionEvent.h"
#include "InputDevice.h"

MonkeyMotionEvent::MonkeyMotionEvent(){
    openInputDevice();
    getDisplaySize(&height, &width);
}
MonkeyMotionEvent::~MonkeyMotionEvent(){
    closeInputDevice();
}

int MonkeyMotionEvent::injectEvent(int randomValue){
    int x = randomValue%width;
    int y = randomValue%height;
    //LOGD("UAPP1", "MonkeyMotionEvent injectEvent random x=%d, y=%d \n",x,y);

    if (randomValue % 100 > 40) { // 滑动和点击事件的权重可调整
        sendTapEvent(x, y);
    } else if (randomValue % 100 > 2){
        int toX = randomValue / 10 % width;
        int toY = randomValue / 10 % height;
        //todo 滑动的duration是否需要定制
        sendSwipeEvent(x, y, toX, toY, 2500);
    } else {
        sendBackEvent();
    }

    return 0;
}
