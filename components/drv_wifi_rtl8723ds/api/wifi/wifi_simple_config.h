/******************************************************************************
 * Copyright (c) 2013-2016 Realtek Semiconductor Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#ifndef __WIFI_SIMPLE_CONFIG_H
#define __WIFI_SIMPLE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************wifi_simple_config.h****************************/
enum sc_result {
	SC_ERROR = -1,	/* default error code*/
	SC_NO_CONTROLLER_FOUND = 1, /* cannot get sta(controller) in the air which starts a simple config session */
	SC_CONTROLLER_INFO_PARSE_FAIL, /* cannot parse the sta's info  */
	SC_TARGET_CHANNEL_SCAN_FAIL, /* cannot scan the target channel */
	SC_JOIN_BSS_FAIL, /* fail to connect to target ap */
	SC_DHCP_FAIL, /* fail to get ip address from target ap */
	 /* fail to create udp socket to send info to controller. note that client isolation
		must be turned off in ap. we cannot know if ap has configured this */
	SC_UDP_SOCKET_CREATE_FAIL,
	SC_TERMINATE,
	SC_SUCCESS,	/* default success code */

};
int SC_send_simple_config_ack(u8 round);

#ifdef __cplusplus
}
#endif

#endif //__WIFI_SIMPLE_CONFIG_H	
