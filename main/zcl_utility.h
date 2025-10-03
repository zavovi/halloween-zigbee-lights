/*
 * SPDX-FileCopyrightText: 2025 MounMovies
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_check.h"
#include "esp_zigbee_core.h"

/*! Maximum length of ManufacturerName string field */
#define ESP_ZB_ZCL_CLUSTER_ID_BASIC_MANUFACTURER_NAME_MAX_LEN 32

/*! Maximum length of ModelIdentifier string field */
#define ESP_ZB_ZCL_CLUSTER_ID_BASIC_MODEL_IDENTIFIER_MAX_LEN 32

/** optional basic manufacturer information */
typedef struct zcl_basic_manufacturer_info_s {
    char *manufacturer_name;
    char *model_identifier;
} zcl_basic_manufacturer_info_t;

/**
 * @brief Adds manufacturer information to the ZCL basic cluster of endpoint
 *
 * @param[in] ep_list The pointer to the endpoint list with @p endpoint_id
 * @param[in] endpoint_id The endpoint identifier indicating where the ZCL basic cluster resides
 * @param[in] info The pointer to the basic manufacturer information
 * @return
 *      - ESP_OK: On success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 */
esp_err_t esp_zcl_utility_add_ep_basic_manufacturer_info(esp_zb_ep_list_t *ep_list, uint8_t endpoint_id, zcl_basic_manufacturer_info_t *info);

#ifdef __cplusplus
} // extern "C"
#endif
