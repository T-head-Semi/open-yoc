#ifndef _SUNXI_HAL_REST_H
#define _SUNXI_HAL_REST_H

#include <sunxi_hal_common.h>

typedef enum {
	HAL_SUNXI_RESET = 0,
	HAL_SUNXI_R_RESET,
	HAL_SUNXI_RESET_NUMBER,
} hal_reset_type_t;

typedef u32 hal_reset_id_t;

#endif /* _SUNXI_HAL_REST_H */
