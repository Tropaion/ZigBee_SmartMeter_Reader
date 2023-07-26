/**
 * @file zb_endpoint.h
 * 
 * @copyright Copyright (c) 2023
 * 
 */

// Multiple inclusion protection
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* General configuration values */
#define HA_DLMS_ENDPOINT    	            1

/**
 * @brief Create endpoint for dlms
 */
void zb_dlms_ep(esp_zb_ep_list_t *esp_zb_ep_list);

#ifdef __cplusplus
}
#endif