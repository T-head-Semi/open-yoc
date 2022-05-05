#include <csi_core.h>
#include <drv/common.h>
#include <drv/codec.h>


extern int sunxi_soundcard_init(void);

csi_error_t csi_codec_init(csi_codec_t *codec, uint32_t idx)
{
    if (sunxi_soundcard_init() != 0) {
        return CSI_ERROR;
    }
    return CSI_OK;
}