/*
 * SPDX-FileCopyrightText: 2026 Roman Fedorov
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include "esp_log.h"
#include "esp_check.h"
#include "pt2258.h"
#include "pt2258_reg.h"

static const char *TAG = "PT2258";

/**
 * @brief Internal PT2258 driver instance structure
 */
typedef struct {
    pt2258_write_cb_t write_cb;
    void *i2c_dev_handle;
} pt2258_i2c_t;

/**
 * @brief Command address codes for PT2258 volume attenuation
 */
typedef struct {
    uint8_t cmd_10db;
    uint8_t cmd_1db;
} pt2258_ch_cmd_t;

/**
 * @brief Command address codes for PT2258 volume attenuation
 * Each entry contains two base bytes for a channel:
 * cmd_10db - Base command for 10 dB attenuation steps (Tens)
 * cmd_1db - Base command for 1 dB attenuation steps (Units)
 */
static const pt2258_ch_cmd_t channel_cmds[PT2258_TOTAL_CHANNELS + 1] = {
    [PT2258_CH_ALL] = { .cmd_10db = PT2258_CMD_ATTEN_10DB_ALL, .cmd_1db = PT2258_CMD_ATTEN_1DB_ALL },
    [PT2258_CH_1]   = { .cmd_10db = PT2258_CMD_ATTEN_10DB_CH1, .cmd_1db = PT2258_CMD_ATTEN_1DB_CH1 },
    [PT2258_CH_2]   = { .cmd_10db = PT2258_CMD_ATTEN_10DB_CH2, .cmd_1db = PT2258_CMD_ATTEN_1DB_CH2 },
    [PT2258_CH_3]   = { .cmd_10db = PT2258_CMD_ATTEN_10DB_CH3, .cmd_1db = PT2258_CMD_ATTEN_1DB_CH3 },
    [PT2258_CH_4]   = { .cmd_10db = PT2258_CMD_ATTEN_10DB_CH4, .cmd_1db = PT2258_CMD_ATTEN_1DB_CH4 },
    [PT2258_CH_5]   = { .cmd_10db = PT2258_CMD_ATTEN_10DB_CH5, .cmd_1db = PT2258_CMD_ATTEN_1DB_CH5 },
    [PT2258_CH_6]   = { .cmd_10db = PT2258_CMD_ATTEN_10DB_CH6, .cmd_1db = PT2258_CMD_ATTEN_1DB_CH6 },
};

/**
 * @brief Write register data to PT2258 via injected transport
 * 
 * @param[in] handle Driver handle
 * @param[in] data Pointer to data buffer to write
 * @param[in] len Number of bytes to write
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
static esp_err_t _write_reg(pt2258_handle_t handle, const uint8_t *data, size_t len)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle pointer");
    pt2258_i2c_t *dev = (pt2258_i2c_t *)handle;
    
    if (!dev || !dev->write_cb) {
        ESP_LOGE(TAG, "Driver uninitialized or missing write callback");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOG_BUFFER_HEX_LEVEL(TAG, data, len, ESP_LOG_DEBUG);

    esp_err_t ret = dev->write_cb(dev->i2c_dev_handle, data, len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C transport write failed (%s)", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t pt2258_create(const pt2258_config_t *cfg, pt2258_handle_t *handle)
{
    ESP_RETURN_ON_FALSE(cfg, ESP_ERR_INVALID_ARG, TAG, "invalid device config pointer");
    ESP_RETURN_ON_FALSE(cfg->write_cb, ESP_ERR_INVALID_ARG, TAG, "missing I2C write callback");
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle pointer");

    ESP_LOGI(TAG, "Initializing %s instance via injected transport", PT2258_CHIP_NAME);

    // Allocate memory for internal driver structure
    pt2258_i2c_t *dev = calloc(1, sizeof(pt2258_i2c_t));
    ESP_RETURN_ON_FALSE(dev, ESP_ERR_NO_MEM, TAG, "failed to allocate memory for driver state");
    
    // Inject dependencies
    dev->write_cb = cfg->write_cb;
    dev->i2c_dev_handle = cfg->i2c_dev_handle;

    // Reset chip registers for initial hardware initialization
    esp_err_t ret = pt2258_clear_registers((pt2258_handle_t)dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Hardware reset failed during creation, cleaning up...");
        free(dev);
        return ret;
    }

    *handle = (pt2258_handle_t)dev;
    ESP_LOGI(TAG, "%s driver initialized successfully", PT2258_CHIP_NAME);    
    return ESP_OK;
}

esp_err_t pt2258_delete(pt2258_handle_t *handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle pointer");
    ESP_RETURN_ON_FALSE(*handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle");
 
    free(*handle);
    *handle = NULL;
    
    ESP_LOGI(TAG, "%s driver instance deleted", PT2258_CHIP_NAME);
    return ESP_OK;
}

esp_err_t pt2258_clear_registers(pt2258_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle");

    uint8_t cmd = PT2258_CMD_CLEAR_REG;
    return _write_reg(handle, &cmd, 1);
}

esp_err_t pt2258_set_attenuation(pt2258_handle_t handle, pt2258_ch_t ch, uint8_t attenuation_db)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle");
    ESP_RETURN_ON_FALSE((uint32_t)ch <= PT2258_TOTAL_CHANNELS, ESP_ERR_INVALID_ARG, TAG, "invalid channel number");
    
    if (attenuation_db > PT2258_MAX_ATTENUATION) {
        ESP_LOGW(TAG, "Attenuation %d dB exceeds maximum, clamping to %d dB", attenuation_db, PT2258_MAX_ATTENUATION);
        attenuation_db = PT2258_MAX_ATTENUATION;
    }

    uint8_t tens = attenuation_db / 10;
    uint8_t units = attenuation_db % 10;

    uint8_t buffer[2] = {
        channel_cmds[ch].cmd_10db | tens,
        channel_cmds[ch].cmd_1db | units
    };

    esp_err_t ret = _write_reg(handle, buffer, 2);
    if (ret == ESP_OK) {
        if (ch == PT2258_CH_ALL) {
            ESP_LOGD(TAG, "Hardware updated: ALL channels set to -%d dB", attenuation_db);
        } else {
            ESP_LOGD(TAG, "Hardware updated: Channel %d set to -%d dB", ch, attenuation_db);
        }
    }
    return ret;
}

esp_err_t pt2258_set_mute(pt2258_handle_t handle, bool mute)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle");

    uint8_t cmd = mute ? PT2258_CMD_MUTE_ON : PT2258_CMD_MUTE_OFF;
    esp_err_t ret = _write_reg(handle, &cmd, 1);

    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "Mute state changed to: %s", mute ? "ON" : "OFF");
    }
    return ret;
}