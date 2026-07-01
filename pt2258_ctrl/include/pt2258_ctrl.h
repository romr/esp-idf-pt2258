/*
 * SPDX-FileCopyrightText: 2026 Roman Fedorov
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "pt2258.h"

/** @name PT2258 Channel Enable Masks */
///@{
#define PT2258_CH1_ENABLE  (1 << 0) /*!< Channel 1 (Pins 1->20) */
#define PT2258_CH2_ENABLE  (1 << 1) /*!< Channel 2 (Pins 2->19) */
#define PT2258_CH3_ENABLE  (1 << 2) /*!< Channel 3 (Pins 3->18) */
#define PT2258_CH4_ENABLE  (1 << 3) /*!< Channel 4 (Pins 8->13) */
#define PT2258_CH5_ENABLE  (1 << 4) /*!< Channel 5 (Pins 9->12) */
#define PT2258_CH6_ENABLE  (1 << 5) /*!< Channel 6 (Pins 10->11) */
///@}

/**
 * @brief Opaque handle structure definition for the PT2258 controller instance
 */
typedef struct pt2258_ctrl_ctx_t *pt2258_ctrl_handle_t;

/**
 * @brief PT2258 controller configuration structure
 */
typedef struct {
    pt2258_handle_t pt2258;         /*!< PT2258 handle */
    uint8_t active_channels_mask;   /*!< Mask of active channels (e.g., PT2258_CH1_ENABLE | PT2258_CH2_ENABLE) */
    uint8_t init_volume;            /*!< Initial master volume at power-on (0-100%) */
    uint8_t offset_limits;          /*!< Offset limits in dB (must be <= PT2258_MAX_ATTENUATION) */
    bool init_mute;                 /*!< Initial mute state */
    bool use_linear_volume;         /*!< Use linear volume scaling (false = logarithmic LUT, true = linear) */
} pt2258_ctrl_config_t;

/**
 * @brief Initialize a PT2258 controller
 * 
 * @param[in] cfg Pointer to the controller configuration structure
 * @param[out] handle Pointer to store the allocated controller instance handle
 * @return
 * - ESP_OK on success
 * - ESP_ERR_NO_MEM if memory allocation fails
 * - ESP_ERR_INVALID_ARG if parameters are invalid
 */
esp_err_t pt2258_ctrl_create(const pt2258_ctrl_config_t *cfg, pt2258_ctrl_handle_t *handle);

/**
 * @brief Free controller resources and delete instance
 * @param[in,out] handle Pointer to the controller handle to be destroyed
 * @return 
 * - ESP_OK on success
 * - ESP_ERR_INVALID_ARG if handle pointer is NULL
 */
esp_err_t pt2258_ctrl_delete(pt2258_ctrl_handle_t *handle);

/**
 * @brief Set channel offset relative to master volume
 * @param[in] handle  Controller instance handle
 * @param[in] channel Channel number (1-6)
 * @param[in] offset  Offset value in dB
 * @return 
 * - ESP_OK on success
 * - ESP_ERR_INVALID_ARG if channel or offset bounds are invalid
 * - ESP_ERR_INVALID_STATE if requested channel is masked out as inactive
 */
esp_err_t pt2258_ctrl_set_offset(pt2258_ctrl_handle_t handle, uint8_t channel, int16_t offset);

/**
 * @brief Reset all channel offsets back to 0 dB (Restore balance)
 * @param[in] handle Controller instance handle
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t pt2258_ctrl_reset_offsets(pt2258_ctrl_handle_t handle);

/**
 * @brief Set master volume percentage for all active channels
 * @param[in] handle Controller instance handle
 * @param[in] volume Volume percentage value (0-100)
 * @return 
 * - ESP_OK on success
 * - ESP_ERR_INVALID_ARG if volume is out of bounds (> 100%)
 */
esp_err_t pt2258_ctrl_set_master_volume(pt2258_ctrl_handle_t handle, uint8_t volume);

/**
 * @brief Increase master volume by a relative percentage step
 * @param[in] handle Controller instance handle
 * @param[in] step_percent Step up value percentage (1-100%)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t pt2258_ctrl_volume_step_up(pt2258_ctrl_handle_t handle, uint8_t step_percent);

/**
 * @brief Decrease master volume by a relative percentage step
 * @param[in] handle Controller instance handle
 * @param[in] step_percent Step down value percentage (1-100%)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t pt2258_ctrl_volume_step_down(pt2258_ctrl_handle_t handle, uint8_t step_percent);

/**
 * @brief Toggle the absolute mute state for all channels
 * @param[in] handle Controller instance handle
 * @param[in] mute Set true to activate global mute, false to restore volume state
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t pt2258_ctrl_set_mute(pt2258_ctrl_handle_t handle, bool mute);

/**
 * @brief Get the actual current attenuation for a specific channel
 * @param[in] handle Controller instance handle
 * @param[in] channel Channel number (1-6)
 * @param[out] attenuation Pointer to store the computed attenuation value (0-79 dB)
 * @return 
 * - ESP_OK on success
 * - ESP_ERR_INVALID_ARG if destination pointer is NULL or channel is out of range
 */
esp_err_t pt2258_ctrl_get_attenuation(pt2258_ctrl_handle_t handle, uint8_t channel, uint8_t *attenuation);

/**
 * @brief Get current channel offset value relative to master volume
 * @param[in] handle  Controller instance handle
 * @param[in] channel Channel number (1-6)
 * @param[out] offset  Pointer to store the channel's offset value in dB
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t pt2258_ctrl_get_offset(pt2258_ctrl_handle_t handle, uint8_t channel, int16_t *offset);

/**
 * @brief Get current tracked master volume percentage
 * @param[in] handle Controller instance handle
 * @param[out] volume Pointer to store the volume percentage status (0-100)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t pt2258_ctrl_get_master_volume(pt2258_ctrl_handle_t handle, uint8_t *volume);

/**
 * @brief Get current tracked global mute state
 * @param[in] handle Controller instance handle
 * @param[out] mute  Pointer to store the boolean mute state status
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t pt2258_ctrl_get_mute(pt2258_ctrl_handle_t handle, bool *mute);

#ifdef __cplusplus
}
#endif