/*
 * SPDX-FileCopyrightText: 2026 Roman Fedorov
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/** @name PT2258 I2C Addresses (8-bit) as in the Datasheet */
///@{
#define PT2258_I2C_ADDR_0_8BIT   (0x80)    /*!< Pins: CODE1=0, CODE2=0 */
#define PT2258_I2C_ADDR_1_8BIT   (0x84)    /*!< Pins: CODE1=0, CODE2=1 */
#define PT2258_I2C_ADDR_2_8BIT   (0x88)    /*!< Pins: CODE1=1, CODE2=0 */
#define PT2258_I2C_ADDR_3_8BIT   (0x8C)    /*!< Pins: CODE1=1, CODE2=1 */
///@}

/** @name PT2258 I2C Addresses (7-bit) */
///@{
#define PT2258_I2C_ADDR_0   (PT2258_I2C_ADDR_0_8BIT >> 1)    /*!< 7-bit address: 0x40 (pins: CODE1=0, CODE2=0) */
#define PT2258_I2C_ADDR_1   (PT2258_I2C_ADDR_1_8BIT >> 1)    /*!< 7-bit address: 0x42 (pins: CODE1=0, CODE2=1) */
#define PT2258_I2C_ADDR_2   (PT2258_I2C_ADDR_2_8BIT >> 1)    /*!< 7-bit address: 0x44 (pins: CODE1=1, CODE2=0) */
#define PT2258_I2C_ADDR_3   (PT2258_I2C_ADDR_3_8BIT >> 1)    /*!< 7-bit address: 0x46 (pins: CODE1=1, CODE2=1) */
///@}

/** @name PT2258 Operational Constants */
///@{
#define PT2258_MIN_ATTENUATION  (0)      /*!< Minimum attenuation of PT2258(0 dB) */
#define PT2258_MAX_ATTENUATION  (79)     /*!< Maximum attenuation of PT2258(-79 dB, silence) */
#define PT2258_TOTAL_CHANNELS   (6)      /*!< Total number of channels on the PT2258 (6) */
///@}

/**
 * @brief PT2258 channel selection
 */
typedef enum {
    PT2258_CH_ALL = 0,   /*!< Master Volume (All channels) */
    PT2258_CH_1   = 1,   /*!< Channel 1 */
    PT2258_CH_2,         /*!< Channel 2 */
    PT2258_CH_3,         /*!< Channel 3 */
    PT2258_CH_4,         /*!< Channel 4 */
    PT2258_CH_5,         /*!< Channel 5 */
    PT2258_CH_6,         /*!< Channel 6 */
} pt2258_ch_t;

/**
 * @brief Type of PT2258 device handle
 *
 */
typedef void *pt2258_handle_t;

/**
 * @brief Type of PT2258 write function callback
 * 
 * @param[in] handle Context pointer (e.g. device descriptor, transport context)
 * @param[in] data Pointer to data buffer to write
 * @param[in] len Number of bytes to write
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
typedef esp_err_t (*pt2258_write_cb_t)(void *handle, const uint8_t *data, size_t len);

/**
 * @brief PT2258 Configuration Structure
 */
typedef struct {
    pt2258_write_cb_t write_cb;  /*!< I2C write callback function */
    void *i2c_dev_handle;        /*!< Context pointer passed to the callback (e.g. i2c_master_dev_handle_t, i2c_bus_device_handle_t) */
} pt2258_config_t;

/**
 * @brief Create PT2258 device handle
 * @param[in] cfg Pointer to the hardware configuration structure
 * @param[out] handle Pointer to the device handle
 * @return 
 * - ESP_OK: Success
 * - ESP_ERR_INVALID_ARG: If cfg or handle is NULL
 * - ESP_ERR_NO_MEM: If memory allocation failed
 */
esp_err_t pt2258_create(const pt2258_config_t *cfg, pt2258_handle_t *handle);

/**
 * @brief Delete PT2258 device handle
 * @param[in] handle Pointer to the device handle pointer (will be set to NULL on success)
 * @return 
 * - ESP_OK: Success
 * - ESP_ERR_INVALID_ARG: Invalid handle
 */
esp_err_t pt2258_delete(pt2258_handle_t *handle);

/**
 * @brief Clear all internal registers of the PT2258 (System Reset)
 * @param[in] handle PT2258 device handle
 * @return 
 * - ESP_OK: Success
 * - ESP_ERR_INVALID_ARG: If handle is NULL
 * - Others: I2C transfer error codes
 */
esp_err_t pt2258_clear_registers(pt2258_handle_t handle);

/**
 * @brief Directly set attenuation in dB for a specific channel or ALL channels
 * @param[in] handle PT2258 device handle
 * @param[in] ch Channel number: PT2258_CH_ALL for ALL channels (Master), PT2258_CH_1-PT2258_CH_6 for CH1-CH6
 * @param[in] attenuation_db Attenuation value from 0 dB (max volume) to 79 dB (silence)
 * @return 
 * - ESP_OK: Success
 * - ESP_ERR_INVALID_ARG: If handle is NULL or ch number is out of bounds (> 6)
 * - Others: I2C transfer error codes
 */
esp_err_t pt2258_set_attenuation(pt2258_handle_t handle, pt2258_ch_t ch, uint8_t attenuation_db);

/**
 * @brief Global Mute control on the chip level
 * @param[in] handle PT2258 device handle
 * @param[in] mute true to mute, false to unmute
 * @return 
 * - ESP_OK: Success
 * - ESP_ERR_INVALID_ARG: If handle is NULL
 * - Others: I2C transfer error codes
 */
esp_err_t pt2258_set_mute(pt2258_handle_t handle, bool mute);

#ifdef __cplusplus
}
#endif