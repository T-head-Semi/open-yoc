#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

// #include <pin_name.h>

/*
 *"wiz_config.h" file is used for CDK system configuration.
 *Makefile compilation needs to keep the file empty.
*/
//#include "wiz_config.h"

/* Allow LOGD function to output log information */
#ifndef CONFIG_DEBUG
#define CONFIG_DEBUG 1
#endif

#if defined(CONFIG_GW_NETWORK_SUPPORT) && CONFIG_GW_NETWORK_SUPPORT
/* You can add user configuration items here. */
#if defined(CONFIG_GW_SMARTLIVING_SUPPORT) && CONFIG_GW_SMARTLIVING_SUPPORT
#define CONFIG_WIFI_SMARTLIVING 1 /* 飞燕配网 */
#define CONFIG_SMARTLIVING_MQTT 1 /* 飞燕mqtt发布及订阅 */
#define CONFIG_SMARTLIVING_DEMO 1    /* 飞燕demo */
#define CONFIG_SMARTLIVING_AT_MODULE 1  /* 飞燕模组 */
#define CONFIG_YOC_SOFTAP_PROV 0 /* YOC配网*/
#else
#define CONFIG_YOC_SOFTAP_PROV 1 /* YOC配网*/
#endif
#else
#if defined(CONFIG_GW_SMARTLIVING_SUPPORT) && CONFIG_GW_SMARTLIVING_SUPPORT
#error "using smartliving should enable network first"
#endif
#endif

#define CONFIG_MANTB_VERSION 4

#define CONSOLE_IDX 0

/* You can add user configuration items here. */
#define CONFIG_GW_FOTA_EN 1

/* PWM输出 */
#define APP_PWM_EN 1

#endif
