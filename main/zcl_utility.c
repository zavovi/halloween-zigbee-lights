/*
 * SPDX-FileCopyrightText: 2025 MounMovies
 *
 */
#include "esp_check.h"
#include "stdio.h"
#include "string.h"
#include "zcl_utility.h"
#include <stdint.h>

static const char *TAG = "ZCL_UTILITY";

esp_err_t esp_zcl_utility_add_ep_basic_manufacturer_info(esp_zb_ep_list_t *ep_list, uint8_t endpoint_id, zcl_basic_manufacturer_info_t *info)
{
    esp_err_t ret = ESP_OK;
    esp_zb_cluster_list_t *cluster_list = NULL;
    esp_zb_attribute_list_t *basic_cluster = NULL;

    cluster_list = esp_zb_ep_list_get_ep(ep_list, endpoint_id);
    ESP_RETURN_ON_FALSE(cluster_list, ESP_ERR_INVALID_ARG, TAG, "Failed to find endpoint id: %d in list: %p", endpoint_id, ep_list);
    basic_cluster = esp_zb_cluster_list_get_cluster(cluster_list, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    ESP_RETURN_ON_FALSE(basic_cluster, ESP_ERR_INVALID_ARG, TAG, "Failed to find basic cluster in endpoint: %d", endpoint_id);
    ESP_RETURN_ON_FALSE((info && info->manufacturer_name), ESP_ERR_INVALID_ARG, TAG, "Invalid manufacturer name");
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, info->manufacturer_name));
    ESP_RETURN_ON_FALSE((info && info->model_identifier), ESP_ERR_INVALID_ARG, TAG, "Invalid model identifier");
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, info->model_identifier));
    return ret;
}
