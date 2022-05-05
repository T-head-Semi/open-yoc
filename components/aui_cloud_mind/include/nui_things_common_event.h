
/* maxiao.maxiao
   define the 'mutex' for diff platform/HW 
   such as:
*/

#ifndef __NUI_THINGS_COMMON_EVENT_H__
#define __NUI_THINGS_COMMON_EVENT_H__

#ifdef __cplusplus 
extern "C"
{ 
#endif

#include <stdio.h>
#include <stdlib.h>

	
typedef enum {
	kNuiThingsCommonEventAuthOk =0,//
	kNuiThingsCommonEventAuthErr,  //鉴权出错
	kNuiThingsCommonEventTokenOk, //获取token出错
	kNuiThingsCommonEventTokenErr, //获取token出错
} NuiThingsCommonEvent;

/*FunNuiThingsCommonEvent() 回调SDK中通用功能消息的接口。
 * user_data用户传入的数据，多次调用，以首次传入时为准。SDK只是透传次指针，因此需要用户全局空间
 * event 回调事件
 * arg   回调对应参数，不同事件对应不同参数
 */
typedef void (*FunNuiThingsCommonEvent)(void * user_data, NuiThingsCommonEvent event, void * arg);



#ifdef __cplusplus 
}
#endif

#endif
