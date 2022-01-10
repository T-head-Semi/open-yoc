#include "MonkeyKeyEvent.h"
#include "InputDevice.h"
#include "ulog/ulog.h"

MonkeyKeyEvent::MonkeyKeyEvent(){
    openInputDevice();
}

MonkeyKeyEvent::~MonkeyKeyEvent(){
    closeInputDevice();
}

int MonkeyKeyEvent::injectEvent( int randomValue){
    int code;
    getTGKeyValue(randomValue, &code);
    sendKeyEvent(code);
    return 0;
}
