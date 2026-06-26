/*
 * SPDX-FileCopyrightText: 2026 Roman Fedorov
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_check.h"
#include "driver/i2c_master.h"
#include "pt2258_esp_idf.h"

static const char *TAG = "PT2258_IDF_ADAPTER";

static esp_err_t _write_cb(void *dev_handle, const uint8_t *data, size_t len)
{
    return i2c_master_transmit((i2c_master_dev_handle_t)dev_handle, data, len, -1);
}

static void _cleanup_cb(void *dev_handle)
{
    i2c_master_bus_rm_device((i2c_master_dev_handle_t)dev_handle);
}

esp_err_t pt2258_esp_idf_create(const pt2258_esp_idf_config_t *cfg, pt2258_handle_t *handle)
{
    ESP_RETURN_ON_FALSE(cfg && handle, ESP_ERR_INVALID_ARG, TAG, "invalid arguments");
    ESP_RETURN_ON_FALSE(cfg->bus_handle, ESP_ERR_INVALID_ARG, TAG, "I2C bus handle is NULL");

    i2c_master_dev_handle_t i2c_dev_handle;
    i2c_device_config_t i2c_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = cfg->i2c_addr,
        .scl_speed_hz = cfg->clk_speed,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(cfg->bus_handle, &i2c_dev_cfg, &i2c_dev_handle), TAG, "Failed to add I2C device");

    pt2258_config_t pt2258_cfg = {
        .transport_ctx = i2c_dev_handle,
        .write_cb = _write_cb,
        .cleanup_cb = _cleanup_cb,
    };
    esp_err_t ret = pt2258_create(&pt2258_cfg, handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create PT2258 driver");
        i2c_master_bus_rm_device(i2c_dev_handle);
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t pt2258_esp_idf_delete(pt2258_handle_t *handle)
{
    ESP_RETURN_ON_FALSE(handle && *handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle");
    return pt2258_delete(handle);
}