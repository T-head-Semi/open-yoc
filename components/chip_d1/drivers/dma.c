#include <csi_core.h>
#include <drv/common.h>
#include <drv/dma.h>
#include <hal_dma.h>

csi_error_t csi_dma_init(csi_dma_t *dma, int8_t ctrl_id)
{
    hal_dma_init();

    return CSI_OK;
}