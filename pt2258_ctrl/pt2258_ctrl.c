/*
 * SPDX-FileCopyrightText: 2026 Roman Fedorov
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_log.h"
#include "esp_check.h"

#include "pt2258_ctrl.h"

#define MAX_OFFSET_LIMIT 78
#define MAX_VOLUME_LIMIT 100

static const char *TAG = "PT2258_CTRL";

struct pt2258_ctrl_ctx_t {
    pt2258_handle_t chip;                                   /*!< PT2258 device handle */
    uint8_t active_channels_mask;                           /*!< Active channels mask */
    uint8_t master_volume;                                  /*!< Master volume (0-100) */
    uint8_t offset_limits;                                  /*!< Offset limits */
    uint8_t channel_attenuation[PT2258_TOTAL_CHANNELS];     /*!< Current attenuation for each channel */
    int16_t channel_offsets[PT2258_TOTAL_CHANNELS];         /*!< Per-channel offset in dB (signed) */
    bool use_linear_volume;                                 /*!< Use linear volume scaling */
    bool is_muted;                                          /*!< Mute state */
};

/**
 * @brief Logarithmic volume lookup table (LUT) for perceptual audio control
 * Maps 0-100% volume to 79-0 dB attenuation
 * @note Array size is exactly 101 to map indices 0 to 100 inclusive.
 * - Index 0   (0%)   -> 79 dB attenuation (Maximum attenuation / Silence)
 * - Index 100 (100%) ->  0 dB attenuation (Minimum attenuation / Max volume)
 */
static const uint8_t volume_lut[101] = {
    79, 79, 79, 78, 78, 78, 77, 77, 77, 76, // 0%   - 9%
    76, 75, 75, 74, 74, 73, 73, 72, 72, 71, // 10%  - 19%
    71, 70, 70, 69, 68, 68, 67, 66, 66, 65, // 20%  - 29%
    64, 64, 63, 62, 62, 61, 60, 59, 59, 58, // 30%  - 39%
    57, 56, 56, 55, 54, 53, 52, 52, 51, 50, // 40%  - 49%
    49, 48, 47, 47, 46, 45, 44, 43, 42, 41, // 50%  - 59%
    40, 39, 38, 37, 36, 35, 34, 33, 32, 31, // 60%  - 69%
    30, 29, 28, 27, 26, 25, 24, 23, 22, 21, // 70%  - 79%
    20, 19, 18, 17, 16, 15, 14, 13, 12, 11, // 80%  - 89%
    10, 9,  8,  7,  6,  5,  4,  3,  2,  1, // 90%  - 99%
    0                                       // 100%
};

/**
 * @brief Check if a channel is active based on the channel mask
 * @param handle Device handle
 * @param channel Channel number (1 - PT2258_TOTAL_CHANNELS)
 * @return true if channel is active, false otherwise
 */
static inline bool _is_channel_active(pt2258_ctrl_handle_t handle, uint8_t channel)
{
    return (channel >= 1 && channel <= PT2258_TOTAL_CHANNELS) && 
           (handle->active_channels_mask & (1 << (channel - 1))) != 0;
}

/**
 * @brief Validate that a channel is active and within valid range
 * @param handle Device handle
 * @param channel Channel number (1 - PT2258_TOTAL_CHANNELS)
 * @return ESP_OK if channel is valid and active, error code otherwise
 */
static inline esp_err_t _validate_active_channel(pt2258_ctrl_handle_t handle, uint8_t channel)
{
    if (!_is_channel_active(handle, channel)) {
        return (channel < 1 || channel > PT2258_TOTAL_CHANNELS) ? ESP_ERR_INVALID_ARG : ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

/**
 * @brief Convert volume percentage to attenuation value
 * @param handle Device handle
 * @param volume Volume percentage (0 - MAX_VOLUME_LIMIT)
 * @return Attenuation value (0 - PT2258_MAX_ATTENUATION dB)
 */
static inline uint8_t _volume_to_attenuation(pt2258_ctrl_handle_t handle, uint8_t volume)
{
    if (volume > MAX_VOLUME_LIMIT) volume = MAX_VOLUME_LIMIT;

    uint8_t attenuation = handle->use_linear_volume ? 
        ((MAX_VOLUME_LIMIT - volume) * PT2258_MAX_ATTENUATION / MAX_VOLUME_LIMIT) :
        volume_lut[volume];

    return (attenuation > PT2258_MAX_ATTENUATION) ? PT2258_MAX_ATTENUATION : attenuation;
}

static esp_err_t _volume_step_internal(pt2258_ctrl_handle_t handle, int16_t step) 
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(step >= -100 && step <= 100, ESP_ERR_INVALID_ARG, TAG, "invalid step");
    
    if (step == 0) {
        return ESP_OK;
    }

    int16_t new_volume = (int16_t)handle->master_volume + step;

    if (new_volume > MAX_VOLUME_LIMIT) {
        new_volume = MAX_VOLUME_LIMIT;
    } else if (new_volume < 0) {
        new_volume = 0;
    }

    return pt2258_ctrl_set_master_volume(handle, (uint8_t)new_volume);
}

/**
 * @brief Update chip with current master volume and channel offsets
 * @param handle Device handle
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t _update(pt2258_ctrl_handle_t handle)
{   
    uint8_t base_attenuation = _volume_to_attenuation(handle, handle->master_volume);

    for (int i = 0; i < PT2258_TOTAL_CHANNELS; i++) {
        if (!_is_channel_active(handle, i + 1)) continue;

        int16_t target = (int16_t)base_attenuation - handle->channel_offsets[i];

        if (target < 0) target = 0;
        else if (target > PT2258_MAX_ATTENUATION) target = PT2258_MAX_ATTENUATION;

        esp_err_t ret = pt2258_set_attenuation(handle->chip, i + 1, (uint8_t)target);
        if (ret != ESP_OK) {
            return ret;
        }
        handle->channel_attenuation[i] = (uint8_t)target;
    }
    return ESP_OK;
}

esp_err_t pt2258_ctrl_create(const pt2258_ctrl_config_t *cfg, pt2258_ctrl_handle_t *out_handle)
{
    ESP_RETURN_ON_FALSE(cfg && cfg->pt2258 && out_handle, ESP_ERR_INVALID_ARG, TAG, "Invalid arguments");
    ESP_RETURN_ON_FALSE(cfg->active_channels_mask !=0, ESP_ERR_INVALID_ARG, TAG, "No active channels");
    ESP_RETURN_ON_FALSE(cfg->init_volume <= MAX_VOLUME_LIMIT, ESP_ERR_INVALID_ARG, TAG, "Invalid init volume");
    ESP_RETURN_ON_FALSE(cfg->offset_limits <= MAX_OFFSET_LIMIT, ESP_ERR_INVALID_ARG, TAG, "Invalid offset limits");

    pt2258_ctrl_handle_t context = calloc(1, sizeof(struct pt2258_ctrl_ctx_t));
    ESP_RETURN_ON_FALSE(context, ESP_ERR_NO_MEM, TAG, "No memory");

    context->chip = cfg->pt2258;
    context->active_channels_mask = cfg->active_channels_mask;
    context->master_volume = cfg->init_volume;
    context->offset_limits = cfg->offset_limits;
    context->use_linear_volume = cfg->use_linear_volume;
    context->is_muted = cfg->init_mute;
    memset(context->channel_offsets, 0, sizeof(context->channel_offsets));
    memset(context->channel_attenuation, PT2258_MAX_ATTENUATION, sizeof(context->channel_attenuation));

    esp_err_t ret = pt2258_set_mute(context->chip, context->is_muted);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set initial mute");
        free(context);
        return ret;
    }

    ret = _update(context);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initial update failed");
        free(context);
        return ret;
    }

    ESP_LOGI(TAG, "Controller initialized successfully");
    *out_handle = context;
    return ESP_OK;
}

esp_err_t pt2258_ctrl_delete(pt2258_ctrl_handle_t *handle)
{
    ESP_RETURN_ON_FALSE(handle && *handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");

    free(*handle);
    *handle = NULL;
    ESP_LOGI(TAG, "Controller deinitialized");
    return ESP_OK;
}

esp_err_t pt2258_ctrl_set_offset(pt2258_ctrl_handle_t handle, uint8_t channel, int16_t offset)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    ESP_RETURN_ON_FALSE(offset >= -(handle->offset_limits) && offset <= handle->offset_limits, ESP_ERR_INVALID_ARG, TAG, "Invalid offset");

    esp_err_t ret = _validate_active_channel(handle, channel);
    if (ret != ESP_OK) {
        return ret;
    }

    handle->channel_offsets[channel-1] = offset;
    return _update(handle);
}

esp_err_t pt2258_ctrl_reset_offsets(pt2258_ctrl_handle_t handle) {
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    memset(handle->channel_offsets, 0, sizeof(handle->channel_offsets));

    esp_err_t ret = _update(handle);
    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "All channel offsets reset to 0 dB (Audio balance restored)");
    } else {
        ESP_LOGE(TAG, "Failed to reset channel offsets: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t pt2258_ctrl_set_master_volume(pt2258_ctrl_handle_t handle, uint8_t volume)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    ESP_RETURN_ON_FALSE(volume <= MAX_VOLUME_LIMIT, ESP_ERR_INVALID_ARG, TAG, "Invalid volume");

    uint8_t saved_volume = handle->master_volume;
    handle->master_volume = volume;

    esp_err_t ret = _update(handle);
    if (ret != ESP_OK) {
        handle->master_volume = saved_volume;
        ESP_LOGE(TAG, "Failed to set master volume: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGD(TAG, "Master volume set to %d%%", volume);
    }
    return ret;
}

esp_err_t pt2258_ctrl_volume_step_up(pt2258_ctrl_handle_t handle, uint8_t step_percent) {
    return _volume_step_internal(handle, (int16_t)step_percent);
}

esp_err_t pt2258_ctrl_volume_step_down(pt2258_ctrl_handle_t handle, uint8_t step_percent) {
    return _volume_step_internal(handle, -(int16_t)step_percent);
}

esp_err_t pt2258_ctrl_set_mute(pt2258_ctrl_handle_t handle, bool mute)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");

    esp_err_t ret = pt2258_set_mute(handle->chip, mute);
    if (ret == ESP_OK) {
        handle->is_muted = mute;
        ESP_LOGD(TAG, "Mute state changed to: %s", mute ? "ON" : "OFF");
    }

    return ret;
}

esp_err_t pt2258_ctrl_get_attenuation(pt2258_ctrl_handle_t handle, uint8_t channel, uint8_t *attenuation)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    ESP_RETURN_ON_FALSE(attenuation, ESP_ERR_INVALID_ARG, TAG, "Invalid attenuation pointer");

    esp_err_t ret = _validate_active_channel(handle, channel);
    if (ret != ESP_OK) return ret;

    *attenuation = handle->channel_attenuation[channel - 1];

    return ESP_OK;
}

esp_err_t pt2258_ctrl_get_offset(pt2258_ctrl_handle_t handle, uint8_t channel, int16_t *offset)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    ESP_RETURN_ON_FALSE(offset, ESP_ERR_INVALID_ARG, TAG, "Invalid offset pointer");

    esp_err_t ret = _validate_active_channel(handle, channel);
    if (ret != ESP_OK) return ret;

    *offset = handle->channel_offsets[channel - 1];

    return ESP_OK;
}

esp_err_t pt2258_ctrl_get_master_volume(pt2258_ctrl_handle_t handle, uint8_t *volume)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    ESP_RETURN_ON_FALSE(volume, ESP_ERR_INVALID_ARG, TAG, "Invalid volume pointer");

    *volume = handle->master_volume;
    return ESP_OK;
}

esp_err_t pt2258_ctrl_get_mute(pt2258_ctrl_handle_t handle, bool *mute)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    ESP_RETURN_ON_FALSE(mute, ESP_ERR_INVALID_ARG, TAG, "Invalid mute pointer");

    *mute = handle->is_muted;
    return ESP_OK;
}