/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef YOC_AT_UART_BASIC_CMD_H
#define YOC_AT_UART_BASIC_CMD_H

#include <sys/types.h>

// basic:
#define AT              {"AT", at_cmd_at}
#define AT_HELP         {"AT+HELP", at_cmd_help}
#define AT_CGMR         {"AT+CGMR", at_cmd_cgmr}
#define AT_FWVER        {"AT+FWVER", at_cmd_fwver}
#define AT_SYSTIME      {"AT+SYSTIME", at_cmd_systime}
#define AT_SAVE         {"AT+SAVE", at_cmd_save}
#define AT_FACTORYW     {"AT-FACTORYW", at_cmd_factory_w}
#define AT_FACTORYR     {"AT+FACTORYR", at_cmd_factory_r}
#define AT_REBOOT       {"AT+REBOOT", at_cmd_reboot}
#define AT_EVENT        {"AT+EVENT", at_cmd_event}
#define AT_ECHO         {"AT+ECHO", at_cmd_echo}
#define AT_SLEEP        {"AT+SLEEP", at_cmd_sleep}
#define AT_MODEL        {"AT+MODEL", at_cmd_model}
#define AT_DEBUG        {"AT+DEBUG", at_cmd_debug_func}

// kv cmd
#define AT_KVGET        {"AT+KVGET", at_cmd_kv_get}
#define AT_KVSET        {"AT+KVSET", at_cmd_kv_set}
#define AT_KVDEL        {"AT+KVDEL", at_cmd_kv_del}
#define AT_KVGETINT     {"AT+KVGETINT", at_cmd_kv_getint}
#define AT_KVSETINT     {"AT+KVSETINT", at_cmd_kv_setint}
#define AT_KVDELINT     {"AT+KVDELINT", at_cmd_kv_delint}

// ali cloud
#if 1
#define AT_ALIMQTT_PUB        {"AT+PUB", at_cmd_alimqtt_pub}
#define AT_ALIMQTT_CONN       {"AT+ALIYUNCONN", at_cmd_alimqtt_conn}
#define AT_ALIMQTT_DISCONN    {"AT+ALIYUNDISCONN", at_cmd_alimqtt_disconn}

#define AT_ALICOAP_PUB        {"AT+PUB", at_cmd_alicoap_pub}
#define AT_ALICOAP_CONN       {"AT+ALIYUNCONN", at_cmd_alicoap_conn}
#define AT_ALICOAP_DISCONN    {"AT+ALIYUNDISCONN", at_cmd_alicoap_disconn}
#endif
#define AT_ALI_PUB              {"AT+PUB", at_cmd_pub}
#define AT_ALI_CONN             {"AT+ALIYUNCONN", at_cmd_aliconn}
#define AT_ALI_DISCONN          {"AT+ALIYUNDISCONN", at_cmd_alidisconn}
//onenet
#define AT_MIPLCREATE           {"AT+MIPLCREATE", at_cmd_onet_miplcreate}
#define AT_MIPLDELETE           {"AT+MIPLDELETE", at_cmd_onet_mipldel}
#define AT_MIPLOPEN             {"AT+MIPLOPEN", at_cmd_onet_miplopen}
#define AT_MIPLADDOBJ           {"AT+MIPLADDOBJ", at_cmd_onet_mipladdobj}
#define AT_MIPLDELOBJ           {"AT+MIPLDELOBJ", at_cmd_onet_mipldelobj}
#define AT_MIPLCLOSE            {"AT+MIPLCLOSE", at_cmd_onet_miplclose}
#define AT_MIPLNOTIFY           {"AT+MIPLNOTIFY", at_cmd_onet_miplnotify}
#define AT_MIPLREADRSP          {"AT+MIPLREADRSP", at_cmd_onet_miplreadrsp}
#define AT_MIPLWRITERSP         {"AT+MIPLWRITERSP", at_cmd_onet_miplwritersp}
#define AT_MIPLEXECUTERSP       {"AT+MIPLEXECUTERSP", at_cmd_onet_miplexecutersp}
#define AT_MIPLOBSERVERSP       {"AT+MIPLOBSERVERSP", at_cmd_onet_miplobserveresp}
#define AT_MIPLDISCOVERRSP      {"AT+MIPLDISCOVERRSP", at_cmd_onet_mipldiscoverresp}
#define AT_MIPLPARAMETERRSP     {"AT+MIPLPARAMETERRSP", at_cmd_onet_miplparameterresp}
#define AT_MIPLUPDATE           {"AT+MIPLUPDATE", at_cmd_onet_miplupdate}
#define AT_MIPLVER              {"AT+MIPLVER", at_cmd_onet_miplver}
#define AT_COPREG               {"AT+COPREG", at_cmd_onet_copreg}

// socket
#define AT_CIPSTART         {"AT+CIPSTART", at_cmd_cip_start}
#define AT_CIPSTOP          {"AT+CIPSTOP", at_cmd_cip_stop}
#define AT_CIPRECVCFG       {"AT+CIPRECVCFG", at_cmd_cip_recv_cfg}
#define AT_CIPID            {"AT+CIPID", at_cmd_cip_id}
#define AT_CIPSTATUS        {"AT+CIPSTATUS", at_cmd_cip_status}
#define AT_CIPSEND          {"AT+CIPSEND", at_cmd_cip_send}
#define AT_CIPSENDPSM       {"AT+CIPSENDPSM", at_cmd_cip_sendpsm}
#define AT_CIPRECV          {"AT+CIPRECV", at_cmd_cip_recv}

// ztw socket
#define AT_ZIPOPEN      {"AT+ZIPOPEN", at_cmd_zip_open}
#define AT_ZIPSEND      {"AT+ZIPSEND", at_cmd_zip_send}
#define AT_ZIPCLOSE     {"AT+ZIPCLOSE", at_cmd_zip_close}
#define AT_ZIPSTAT      {"AT+ZIPSTAT", at_cmd_zip_stat}
#define AT_ZDTMODE      {"AT+ZDTMODE", at_cmd_zdt_mode}

// hisi socket
#define AT_NSOCR        {"AT+NSOCR", at_cmd_nsocr}
#define AT_NSOST        {"AT+NSOST", at_cmd_nsost}
#define AT_NSOSTF       {"AT+NSOSTF", at_cmd_nsostf}
#define AT_NSORF        {"AT+NSORF", at_cmd_nsorf}
#define AT_NSOCL        {"AT+NSOCL", at_cmd_nsocl}


// CHIP_ZX297100
#define AT_ZNVSET       {"AT+ZNVSET", at_cmd_znvset}
#define AT_ZNVGET       {"AT+ZNVGET", at_cmd_znvget}
#define AT_RAMDUMP      {"AT^RAMDUMP", at_cmd_ramdump}
#define AT_EXTRTC       {"AT+EXTRTC", at_cmd_extrtc}
#define AT_LPMENTER     {"AT+LPMENTER", at_cmd_quick_lpm}

// AMT
#define AT_ZFLAG            {"AT+ZFLAG", at_cmd_zflag}
#define AT_AMTDEMO          {"AT+AMTDEMO", at_cmd_amtdemo}
#define AT_BOARDNUM         {"AT+BOARDNUM", at_cmd_boardnum}
#define AT_MSN              {"AT+MSN", at_cmd_msn}
#define AT_PRODTEST         {"AT+PRODTEST", at_cmd_prodtest}
#define AT_RTESTINFO        {"AT+RTESTINFO", at_cmd_rtestinfo}
#define AT_ZVERSIONTYPE     {"AT+ZVERSIONTYPE", at_cmd_zversiontype}
#define AT_PLATFORM         {"AT+PLATFORM", at_cmd_chip_platform}

// fota
#define AT_FOTASTART        {"AT+FOTASTART", at_cmd_fotastart}
#define AT_FOTAGETFULL      {"AT+FOTAGETFULL", at_cmd_fotagetfull}
#define AT_FOTASTOP         {"AT+FOTASTOP", at_cmd_fotastop}
#define AT_FOTAGETDIFF      {"AT+FOTAGETDIFF", at_cmd_fotagetdiff}

#define AT_OTASTART         {"AT+OTASTART",     at_cmd_otastart}
#define AT_OTASTOP          {"AT+OTASTOP",      at_cmd_otastop}
#define AT_OTAFINISH        {"AT+OTAFINISH",    at_cmd_otafinish}
#define AT_OTAGETINFO       {"AT+OTAGETINFO",   at_cmd_otagetinfo}
#define AT_OTAPOST          {"AT+OTAPOST",      at_cmd_otapost}

#ifdef COFNIG_BT
//BT
#define AT_BT_NAME          {"AT+BTNAME",at_cmd_bt_name}
#define AT_BT_ADV           {"AT+BTADV", at_cmd_bt_adv}
#define AT_BT_AUTO_ADV      {"AT+BTAUTOADV",at_cmd_bt_adv_def}
#define AT_BT_CONN          {"AT+BTCONN",at_cmd_bt_conn}
#define AT_BT_CONN_UPDATE   {"AT+BTCONNUPDATE",at_cmd_bt_conn_update}
#define AT_BT_AUTO_CONN     {"AT+BTAUTOCONN",at_cmd_bt_conn_def}
#define AT_BT_TX            {"AT+BTTX",  at_cmd_bt_tx}
#define AT_BT_ROLE          {"AT+BTROLE",at_cmd_bt_role}
#define AT_BT_REBOOT        {"AT+REBOOT",at_cmd_bt_reboot}
#define AT_BT_RST           {"AT+BTRST",at_cmd_bt_rst}
#define AT_BT_SLEEP         {"AT+BTSLEEP",at_cmd_bt_sleep_mode}
#define AT_BT_TXPOWER       {"AT+BTTXPOW",at_cmd_bt_tx_power}
#define AT_BT_FWVER         {"AT+BTFWVER",at_cmd_bt_fwver}
#define AT_BT_BAUD          {"AT+BAUD",at_cmd_bt_baud}
#define AT_BT_MAC           {"AT+BTMAC",at_cmd_bt_mac}
#define AT_BT_FIND          {"AT+BTFIND",at_cmd_bt_find}
#define AT_BT_DISCONN       {"AT+BTDISCONN",at_cmd_bt_disconn}
#define AT_BT_FIND          {"AT+BTFIND",at_cmd_bt_find}
#endif




#ifdef CONFIG_BT_MESH
//MESH
#define AT_BTMESH_LOG_LEVEL     {"AT+LOGLEVEL",at_cmd_btmesh_log}
#define AT_BTMESH_REBOOT        {"AT+IREBOOT",at_cmd_btmesh_reboot}
#define AT_BTMESH_QUERY_STA     {"AT+MESHQUERYSTA",at_cmd_btmesh_query_sta}
#define AT_BTMESH_RST_DEV      {"AT+RST",at_cmd_btmesh_rst_dev}
#define AT_BTMESH_CLEAR_RPL     {"AT+MESHCLEARRPL",at_cmd_btmesh_rpl_clear}
#define AT_BTMESH_AT_EN            {"AT+MESHAT",at_cmd_btmesh_at_enable}
#define AT_BTMESH_PROV_CONFIG       {"AT+MESHPROVCONFIG",at_cmd_btmesh_prov_config}
#define AT_BTMESH_PROV_AUTO_PROV    {"AT+MESHPROVAUTOPROV",at_cmd_btmesh_prov_auto_prov}
#define AT_BTMESH_PROV_EN       {"AT+MESHPROVEN",at_cmd_btmesh_prov_en}
#define AT_BTMESH_PROV_FILTER_DEV {"AT+MESHPROVFILTERDEV", at_cmd_btmesh_prov_set_uuid_filter}
#define AT_BTMESH_PROV_SHOW_DEV {"AT+MESHPROVSHOWDEV", at_cmd_btmesh_prov_show_dev}
#define AT_BTMESH_GET_NODE_INFO {"AT+MESHGETNODEINFO",at_cmd_btmesh_prov_get_node_info}
#define AT_BTMESH_ADD_NODE      {"AT+MESHADDNODE",at_cmd_btmesh_prov_add_node}
#define AT_BTMESH_ADDDEV        {"AT+MESHADDDEV",at_cmd_btmesh_prov_add_dev}
#define AT_BTMESH_AUTOCONFIG    {"AT+MESHAUTOCONFIG",at_cmd_btmesh_prov_node_auto_config}
#define AT_BTMESH_GET_NODE_VERSION    {"AT+MESHGETNODEVER",at_cmd_btmesh_get_node_version}
#define AT_BTMESH_DELDEV        {"AT+MESHDELDEV",at_cmd_btmesh_prov_del_dev}
#define AT_BTMESH_OOB           {"AT+MESHOOB",at_cmd_btmesh_prov_add_oob}
#define AT_BTMESH_NETKEY_GET    {"AT+MESHGETNETKEY",at_cmd_btmesh_prov_netkey_get}
#define AT_BTMESH_NETKEY_SET    {"AT+MESHSETNETKEY",at_cmd_btmesh_prov_netkey_set}
#define AT_BTMESH_APPKEY_GET    {"AT+MESHGETAPPKEY",at_cmd_btmesh_prov_appkey_get}
#define AT_BTMESH_APPKEY_SET    {"AT+MESHSETAPPKEY",at_cmd_btmesh_prov_appkey_set}
#define AT_BTMESH_APPKEY_ADD    {"AT+MESHADDAPPKEY",at_cmd_btmesh_appkey_add}
#define AT_BTMESH_APPKEY_BIND   {"AT+MESHBINDAPPKEY",at_cmd_btmesh_appkey_bind}
#define AT_BTMESH_APPKEY_UNBIND   {"AT+MESHUNBINDAPPKEY",at_cmd_btmesh_appkey_unbind}
#define AT_BTMESH_CFG_RELAY     {"AT+MESHRELAY",at_cmd_btmesh_relay}
#define AT_BTMESH_CFG_COMP_GET  {"AT+MESHGETCOMP",at_cmd_btmesh_get_comp}
#define AT_BTMESH_CFG_PROXY     {"AT+MESHPROXY",at_cmd_btmesh_proxy}
#define AT_BTMESH_CFG_FRIEND    {"AT+MESHFRIEND",at_cmd_btmesh_friend}
#define AT_BTMESH_PUB_SET           {"AT+MESHSETPUB",at_cmd_btmesh_pub}
#define AT_BTMESH_SUB_SET           {"AT+MESHSETSUB",at_cmd_btmesh_sub}
#define AT_BTMESH_SUB_GET       {"AT+MESHGETSUB",at_cmd_btmesh_sub_get}
#define AT_BTMESH_PUB_GET       {"AT+MESHGETPUB",at_cmd_btmesh_pub_get}
#define AT_BTMESH_SUB_DEL       {"AT+MESHDELSUB",at_cmd_btmesh_sub_del}
#define AT_BTMESH_SUBLIST_OVERWRITE    {"AT+MESHSUBLISTOVERWRITE",at_cmd_btmesh_sub_overwrite}
#define AT_BTMESH_RST           {"AT+MESHRST",at_cmd_btmesh_rst}
#define AT_BTMESH_ONOFF         {"AT+MESHONOFF",at_cmd_btmesh_onoff}
#define AT_BTMESH_LEVEL         {"AT+MESHLEVEL",at_cmd_btmesh_level}
#define AT_BTMESH_LEVEL_MOVE    {"AT+MESHLEVELMOVE",at_cmd_btmesh_level_move}
#define AT_BTMESH_LEVEL_DELTA   {"AT+MESHLEVELDELTA",at_cmd_btmesh_level_move_delta}
#define AT_BTMESH_LIGHTNESS     {"AT+MESHLIGHTNESS",at_cmd_btmesh_lightness}
#define AT_BTMESH_LIGHTNESS_LIN {"AT+MESHLIGHTNESSLIN",at_cmd_btmesh_lightness_linear}
#define AT_BTMESH_LIGHTNESS_RANGE     {"AT+MESHLIGHTNESSRANGE",at_cmd_btmesh_lightness_range}
#define AT_BTMESH_LIGHTNESS_DEF     {"AT+MESHLIGHTNESSDEF",at_cmd_btmesh_lightness_def}
#define AT_BTMESH_LIGHT_CTL     {"AT+MESHCTL",at_cmd_btmesh_light_ctl}
#define AT_BTMESH_LIGHT_CTL_TEMP   {"AT+MESHCTLTEMP",at_cmd_btmesh_light_ctl_temp}
#define AT_BTMESH_LIGHT_CTL_RANGE    {"AT+MESHCTLTEMPRANGE",at_cmd_btmesh_light_ctl_range}
#define AT_BTMESH_LIGHT_CTL_DEF   {"AT+MESHCTLDEF",at_cmd_btmesh_light_ctl_def}
#define AT_BTMESH_TRS           {"AT+MESHTRS",at_cmd_btmesh_vendor_send}
#if defined(CONFIG_OTA_SERVER)
#define AT_BTMESH_ADD_OTA_DEV   {"AT+MESHADDOTADEV",at_cmd_btmesh_add_ota_dev}
#endif
#define AT_BTMESH_PROV_FILTER_MAC     {"AT+MESHPROVMACFILTER",at_cmd_btmesh_prov_set_mac_filter}
#define AT_BTMESH_PROV_FILTER_MAC_ADD {"AT+MESHPROVMACFILTERADD",at_cmd_btmesh_prov_add_mac_filter_dev}
#define AT_BTMESH_PROV_FILTER_MAC_RM  {"AT+MESHPROVMACFILTERRM",at_cmd_btmesh_prov_rm_mac_filter_dev}
#define AT_BTMESH_PROV_FILTER_MAC_CLEAR  {"AT+MESHPROVMACFILTERCLR",at_cmd_btmesh_prov_clear_mac_filter}
#ifdef CONFIG_MESH_LPM
#define AT_BTMESH_PROV_SET_NODE_LPM_FLAG  {"AT+MESHPROVSETNODELPM",at_cmd_btmesh_prov_set_node_lpm_flag}
#endif

#endif


#define AT_NULL             {NULL,NULL}

/* basic cmd */
void at_cmd_at(char *cmd, int type, char *data);
void at_cmd_help(char *cmd, int type, char *data);
void at_cmd_cgmr(char *cmd, int type, char *data);
void at_cmd_fwver(char *cmd, int type, char *data);
void at_cmd_systime(char *cmd, int type, char *data);
void at_cmd_save(char *cmd, int type, char *data);
void at_cmd_factory_w(char *cmd, int type, char *data);
void at_cmd_factory_r(char *cmd, int type, char *data);
void at_cmd_reboot(char *cmd, int type, char *data);
void at_cmd_event(char *cmd, int type, char *data);
void at_cmd_echo(char *cmd, int type, char *data);
void at_cmd_model(char *cmd, int type, char *data);
void at_cmd_sleep(char *cmd, int type, char *data);
void at_cmd_debug_func(char *cmd, int type, char *data);

#ifdef COFNIG_BT
/*bt cmd*/
void at_cmd_bt_mac(char *cmd, int type, char *data);
void at_cmd_bt_baud(char *cmd, int type, char *data);
void at_cmd_bt_find(char *cmd, int type, char *data);
void at_cmd_bt_disconn(char *cmd, int type, char *data);

void at_cmd_bt_name(char *cmd, int type, char *data);
void at_cmd_bt_adv(char *cmd, int type, char *data);
void at_cmd_bt_adv_def(char *cmd, int type, char *data);
void at_cmd_bt_conn(char *cmd, int type, char *data);
void at_cmd_bt_conn_def(char *cmd, int type, char *data);
void at_cmd_bt_conn_update(char *cmd, int type, char *data);
void at_cmd_bt_tx(char *cmd, int type, char *data);
void at_cmd_bt_role(char *cmd, int type, char *data);
void at_cmd_bt_reboot(char *cmd, int type, char *data);
void at_cmd_bt_rst(char *cmd, int type, char *data);
void at_cmd_bt_sleep_mode(char *cmd, int type, char *data);
void at_cmd_bt_tx_power(char *cmd, int type, char *data);
void at_cmd_bt_fwver(char *cmd, int type, char *data);
void at_cmd_bt_mac(char *cmd, int type, char *data);
void at_cmd_bt_baud(char *cmd, int type, char *data);
void at_cmd_bt_find(char *cmd, int type, char *data);
void at_cmd_bt_fota(char *cmd, int type, char *data);
#endif

#ifdef CONFIG_BT_MESH
/* mesh cmd */
void at_cmd_btmesh_log(char *cmd, int type, char *data);
void at_cmd_btmesh_rst_dev(char * cmd, int type, char * data);
void at_cmd_btmesh_prov_config(char *cmd, int type, char *data);
void at_cmd_btmesh_prov_auto_prov(char *cmd, int type, char *data);
void at_cmd_btmesh_prov_en(char *cmd, int type, char *data);
void at_cmd_btmesh_rpl_clear(char * cmd, int type, char * data);
void at_cmd_btmesh_at_enable(char *cmd, int type, char *data);
void at_cmd_btmesh_prov_set_uuid_filter(char *cmd, int type, char *data);
void at_cmd_btmesh_prov_show_dev(char *cmd, int type, char *data);
void at_cmd_btmesh_prov_add_dev(char *cmd, int type, char *data);
void at_cmd_btmesh_prov_node_auto_config(char * cmd, int type, char * data);
void at_cmd_btmesh_get_node_version(char * cmd, int type, char * data);
void at_cmd_btmesh_prov_del_dev(char *cmd, int type, char *data);
void at_cmd_btmesh_prov_add_oob(char *cmd, int type, char *data);
void at_cmd_btmesh_prov_netkey_get(char * cmd, int type, char * data);
void at_cmd_btmesh_prov_netkey_set(char * cmd, int type, char * data);
void at_cmd_btmesh_prov_appkey_get(char * cmd, int type, char * data);
void at_cmd_btmesh_prov_appkey_set(char * cmd, int type, char * data);
void at_cmd_btmesh_prov_get_node_info(char *cmd, int type, char *data);
void at_cmd_btmesh_prov_add_node(char *cmd, int type, char *data);
void at_cmd_btmesh_appkey_add(char *cmd, int type, char *data);
void at_cmd_btmesh_appkey_bind(char *cmd, int type, char *data);
void at_cmd_btmesh_appkey_unbind(char *cmd, int type, char *data);
void at_cmd_btmesh_local_key_get(char * cmd, int type, char * data);
void at_cmd_btmesh_pub(char *cmd, int type, char *data);
void at_cmd_btmesh_sub(char *cmd, int type, char *data);
void at_cmd_btmesh_sub_get(char * cmd, int type, char * data);
void at_cmd_btmesh_pub_get(char * cmd, int type, char * data);
void at_cmd_btmesh_get_comp(char *cmd, int type, char *data);
void at_cmd_btmesh_sub_del(char *cmd, int type, char *data);
void at_cmd_btmesh_sub_overwrite(char * cmd, int type, char * data);
void at_cmd_btmesh_rst(char *cmd, int type, char *data);
void at_cmd_btmesh_onoff(char *cmd, int type, char *data);
void at_cmd_btmesh_level(char *cmd, int type, char *data);
void at_cmd_btmesh_level_move(char *cmd, int type, char *data);
void at_cmd_btmesh_level_move_delta(char *cmd, int type, char *data);
void at_cmd_btmesh_lightness(char *cmd, int type, char *data);
void at_cmd_btmesh_lightness_linear(char *cmd, int type, char *data);
void at_cmd_btmesh_lightness_range(char *cmd, int type, char *data);
void at_cmd_btmesh_lightness_def(char *cmd, int type, char *data);
void at_cmd_btmesh_light_ctl(char *cmd, int type, char *data);
void at_cmd_btmesh_light_ctl_temp(char *cmd, int type, char *data);
void at_cmd_btmesh_light_ctl_range(char *cmd, int type, char *data);
void at_cmd_btmesh_light_ctl_def(char *cmd, int type, char *data);
void at_cmd_btmesh_vendor_send(char *cmd, int type, char *data);
void at_cmd_btmesh_reboot(char * cmd, int type, char * data);
void at_cmd_btmesh_query_sta(char * cmd, int type, char * data);
void at_cmd_btmesh_relay(char * cmd, int type, char * data);
void at_cmd_btmesh_proxy(char * cmd, int type, char * data);
void at_cmd_btmesh_friend(char * cmd, int type, char * data);
#if defined(CONFIG_OTA_SERVER)
void at_cmd_btmesh_add_ota_dev(char * cmd, int type, char * data);
#endif
void at_cmd_btmesh_prov_set_mac_filter(char *cmd, int type, char *data);
void at_cmd_btmesh_prov_add_mac_filter_dev(char *cmd, int type, char *data);
void at_cmd_btmesh_prov_rm_mac_filter_dev(char *cmd, int type, char *data);
void at_cmd_btmesh_prov_clear_mac_filter(char *cmd, int type, char *data);


#ifdef CONFIG_MESH_LPM
void at_cmd_btmesh_prov_set_node_lpm_flag(char *cmd, int type, char *data);
#endif

#endif

/* service cmd */
void at_cmd_fotastart(char *cmd, int type, char *data);
void at_cmd_fotastop(char *cmd, int type, char *data);
void at_cmd_fotagetfull(char *cmd, int type, char *data);
void at_cmd_fotagetdiff(char *cmd, int type, char *data);
void at_cmd_fota(char *cmd, int type, char *data);
void at_cmd_fotaurl(char *cmd, int type, char *data);
void at_cmd_otastart(char *cmd, int type, char *data);
void at_cmd_otastop(char *cmd, int type, char *data);
void at_cmd_otafinish(char *cmd, int type, char *data);
void at_cmd_otagetinfo(char *cmd, int type, char *data);
void at_cmd_otapost(char *cmd, int type, char *data);
void at_cmd_kv_set(char *cmd, int type, char *data);
void at_cmd_kv_get(char *cmd, int type, char *data);
void at_cmd_kv_del(char *cmd, int type, char *data);
void at_cmd_kv_setint(char *cmd, int type, char *data);
void at_cmd_kv_getint(char *cmd, int type, char *data);
void at_cmd_kv_delint(char *cmd, int type, char *data);


/* cloud cmd */
void at_cmd_pub(char *cmd, int type, char *data);
void at_cmd_aliconn(char *cmd, int type, char *data);
void at_cmd_alidisconn(char *cmd, int type, char *data);

void at_cmd_alicoap_pub(char *cmd, int type, char *data);
void at_cmd_alicoap_conn(char *cmd, int type, char *data);
void at_cmd_alicoap_disconn(char *cmd, int type, char *data);

void at_cmd_alimqtt_pub(char *cmd, int type, char *data);
void at_cmd_alimqtt_conn(char *cmd, int type, char *data);
void at_cmd_alimqtt_disconn(char *cmd, int type, char *data);
//void at_setup_cmd_conntimeout(uint8_t id, uint16_t len, uint8_t *data);
//void at_query_cmd_conntimeout(uint8_t id);



void at_cmd_cip_start(char *cmd, int type, char *data);
void at_cmd_cip_stop(char *cmd, int type, char *data);
void at_cmd_cip_recv_cfg(char *cmd, int type, char *data);
void at_cmd_cip_id(char *cmd, int type, char *data);
void at_cmd_cip_status(char *cmd, int type, char *data);
void at_cmd_cip_send(char *cmd, int type, char *data);
#ifdef CONFIG_YOC_LPM
void at_cmd_cip_sendpsm(char *cmd, int type, char *data);
#endif
void at_cmd_cip_recv(char *cmd, int type, char *data);


#ifdef CONFIG_FOTA
void at_cmd_fotastart(char *cmd, int type, char *data);
void at_cmd_fotagetfull(char *cmd, int type, char *data);
void at_cmd_fotastop(char *cmd, int type, char *data);
void at_cmd_fotagetdiff(char *cmd, int type, char *data);
#endif

// onenet AT command
void at_cmd_onet_miplcreate(char *cmd, int type, char *data);
void at_cmd_onet_mipldel(char *cmd, int type, char *data);
void at_cmd_onet_miplopen(char *cmd, int type, char *data);
void at_cmd_onet_mipladdobj(char *cmd, int type, char *data);
void at_cmd_onet_mipldelobj(char *cmd, int type, char *data);
void at_cmd_onet_miplclose(char *cmd, int type, char *data);
void at_cmd_onet_miplnotify(char *cmd, int type, char *data);
void at_cmd_onet_miplreadrsp(char *cmd, int type, char *data);
void at_cmd_onet_miplwritersp(char *cmd, int type, char *data);
void at_cmd_onet_miplexecutersp(char *cmd, int type, char *data);
void at_cmd_onet_miplobserveresp(char *cmd, int type, char *data);
void at_cmd_onet_mipldiscoverresp(char *cmd, int type, char *data);
void at_cmd_onet_miplparameterresp(char *cmd, int type, char *data);
void at_cmd_onet_miplupdate(char *cmd, int type, char *data);
void at_cmd_onet_miplver(char *cmd, int type, char *data);
void at_cmd_onet_copreg(char *cmd, int type, char *data);

// ZTW AT command
void at_cmd_zip_open(char *cmd, int type, char *data);
void at_cmd_zip_send(char *cmd, int type, char *data);
void at_cmd_zip_close(char *cmd, int type, char *data);
void at_cmd_zip_stat(char *cmd, int type, char *data);
void at_cmd_zdt_mode(char *cmd, int type, char *data);

// HISI AT command
void at_cmd_nsocr(char *cmd, int type, char *data);
void at_cmd_nsost(char *cmd, int type, char *data);
void at_cmd_nsostf(char *cmd, int type, char *data);
void at_cmd_nsorf(char *cmd, int type, char *data);
void at_cmd_nsocl(char *cmd, int type, char *data);

#if defined(CONFIG_CHIP_ZX297100)
// ZX297100 AT command
void at_cmd_znvset(char *cmd, int type, char *data);
void at_cmd_znvget(char *cmd, int type, char *data);
void at_cmd_ramdump(char *cmd, int type, char *data);
#ifdef CONFIG_YOC_LPM
void at_cmd_extrtc(char *cmd, int type, char *data);
void at_cmd_quick_lpm(char *cmd, int type, char *data);
#endif
#ifdef CONFIG_AMT
void at_cmd_zflag(char *cmd, int type, char *data);
void at_cmd_amtdemo(char *cmd, int type, char *data);
void at_cmd_boardnum(char *cmd, int type, char *data);
void at_cmd_msn(char *cmd, int type, char *data);
void at_cmd_prodtest(char *cmd, int type, char *data);
void at_cmd_rtestinfo(char *cmd, int type, char *data);
void at_cmd_zversiontype(char *cmd, int type, char *data);
void at_cmd_chip_platform(char *cmd, int type, char *data);
#endif
#endif

#endif
