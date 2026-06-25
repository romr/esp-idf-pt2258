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
#include "i2c_bus.h"

/**
 * @brief PT2258 Configuration Structure
 */
typedef struct {
    i2c_bus_handle_t bus_handle;    /*!< I2C bus handle */
    uint8_t i2c_addr;               /*!< I2C address */
} pt2258_esp_adf_config_t;

/**
 * @brief Create PT2258 device handle
 * @param[in] cfg Pointer to the hardware configuration structure
 * @param[out] handle Pointer to the device handle
 * @return 
 * - ESP_OK: Success
 * - ESP_ERR_INVALID_ARG: If cfg or handle is NULL
 * - ESP_ERR_NO_MEM: If memory allocation failed
 */
esp_err_t pt2258_esp_adf_create(const pt2258_esp_adf_config_t *cfg, pt2258_handle_t *handle);

/**
 * @brief Delete PT2258 device handle
 * @param[in] handle Pointer to the device handle pointer (will be set to NULL on success)
 * @return 
 * - ESP_OK: Success
 * - ESP_ERR_INVALID_ARG: Invalid handle
 */
esp_err_t pt2258_esp_adf_delete(pt2258_handle_t *handle);

#ifdef __cplusplus
}
#endif