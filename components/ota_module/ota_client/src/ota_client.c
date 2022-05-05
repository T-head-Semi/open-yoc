#include "ota_module.h"
#include "ais_ota/ais_ota_client.h"
#include  "mesh_ota/mesh_ota_client.h"
#include "ulog/ulog.h"

#define TAG  "OTA_CLIENT"


int ota_client_init()
{
    int ret = 0;
	uint8_t protocol_choose_flag = 0;
#if defined(CONFIG_OTA_CLIENT_PROCTOCOL_AIS) && CONFIG_OTA_CLIENT_PROCTOCOL_AIS > 0
    ret = ais_ota_client_init();
    if(ret) {
		LOGE(TAG,"Ais ota client init faild %d",ret);
		return ret;
	}
	protocol_choose_flag = 1;
#endif
#if defined(CONFIG_OTA_CLIENT_PROCTOCOL_MESH) && CONFIG_OTA_CLIENT_PROCTOCOL_MESH > 0
	ret = mesh_ota_client_init();
    if(ret) {
	  LOGE(TAG,"Mesh ota client init faild %d",ret);
	  return ret;
	}
	protocol_choose_flag = 1;
#endif
    if(!protocol_choose_flag) {
	   LOGE(TAG,"No ota proctocol choosed for ota client",ret);
       return -1;
	}
	return 0;
}





