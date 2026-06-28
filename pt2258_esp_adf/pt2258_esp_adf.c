/*
 * SPDX-FileCopyrightText: 2026 Roman Fedorov
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include "esp_log.h"
#include "esp_check.h"
#include "pt2258_esp_adf.h"

static const char *TAG = "PT2258_ADF_ADAPTER";

/**
 * @brief Internal PT2258 driver instance structure
 */
typedef struct {
    i2c_bus_handle_t bus_handle;
    uint8_t i2c_addr;
} i2c_transport_ctx_t;

/**
 * @brief Write callback for PT2258 driver
 * @param[in] dev_handle Pointer to the transport context
 * @param[in] data       Pointer to the data to write
 * @param[in] len        Length of the data to write
 * @return esp_err_t ESP_OK on success, otherwise an error code
 */
static esp_err_t _write_cb(void *dev_handle, const uint8_t *data, size_t len)
{
    i2c_transport_ctx_t *ctx = (i2c_transport_ctx_t *)dev_handle;
    return i2c_bus_write_data(ctx->bus_handle, ctx->i2c_addr, (uint8_t *)data, len);
}

/**
 * @brief Cleanup callback for PT2258 driver
 * @param[in] dev_handle Pointer to the transport context
 */
static void _cleanup_cb(void *dev_handle)
{
    free(dev_handle);
}

esp_err_t pt2258_esp_adf_create(const pt2258_esp_adf_config_t *cfg, pt2258_handle_t *handle)
{
    ESP_RETURN_ON_FALSE(cfg && handle, ESP_ERR_INVALID_ARG, TAG, "invalid arguments");
    ESP_RETURN_ON_FALSE(cfg->bus_handle, ESP_ERR_INVALID_ARG, TAG, "I2C bus handle is NULL");

    i2c_transport_ctx_t *ctx = calloc(1, sizeof(i2c_transport_ctx_t));
    ESP_RETURN_ON_FALSE(ctx, ESP_ERR_NO_MEM, TAG, "Failed to allocate transport context");

    ctx->bus_handle = cfg->bus_handle;
    ctx->i2c_addr = cfg->i2c_addr;

    pt2258_config_t pt2258_cfg = {
        .transport_ctx = ctx,
        .write_cb = _write_cb,
        .cleanup_cb = _cleanup_cb,
    };

    esp_err_t ret = pt2258_create(&pt2258_cfg, handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create PT2258 driver");
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t pt2258_esp_adf_delete(pt2258_handle_t *handle)
{
    ESP_RETURN_ON_FALSE(handle && *handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle");
    return pt2258_delete(handle);
}