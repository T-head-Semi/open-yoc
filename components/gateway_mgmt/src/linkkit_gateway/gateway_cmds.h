#ifndef __GATEWAY_CMDS_H__
#define __GATEWAY_CMDS_H__

#define KV_KEY_PK "pk"
#define KV_KEY_PS "ps"
#define KV_KEY_DN "dn"
#define KV_KEY_DS "ds"

#define MAX_KEY_LEN (6)

extern int gateway_register_cmds(void);
extern void gateway_start_netmgr(void *p);
extern void gateway_open_dev_ap(void *p);
extern void gateway_awss_reset(void);
extern void gateway_do_awss_active(void);

#endif
