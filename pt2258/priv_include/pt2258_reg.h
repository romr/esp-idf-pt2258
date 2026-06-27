/*
 * SPDX-FileCopyrightText: 2026 Roman Fedorov
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define PT2258_CHIP_NAME            "PT2258"    /**< chip name */

#define PT2258_CMD_CLEAR_REG        0xC0        /**< Command to clear and reset all internal registers to default state */
#define PT2258_CMD_MUTE_ON          0xF9        /**< Command to enable Mute mode (cuts off audio for all channels) */
#define PT2258_CMD_MUTE_OFF         0xF8        /**< Command to disable Mute mode (restores previous volume levels) */

#define PT2258_CMD_ATTEN_10DB_ALL   0xD0        /**< Base 10dB attenuation code for ALL six channels simultaneously */
#define PT2258_CMD_ATTEN_10DB_CH1   0x80        /**< Base 10dB attenuation code for Channel 1 */
#define PT2258_CMD_ATTEN_10DB_CH2   0x40        /**< Base 10dB attenuation code for Channel 2 */
#define PT2258_CMD_ATTEN_10DB_CH3   0x00        /**< Base 10dB attenuation code for Channel 3 */
#define PT2258_CMD_ATTEN_10DB_CH4   0x20        /**< Base 10dB attenuation code for Channel 4 */
#define PT2258_CMD_ATTEN_10DB_CH5   0x60        /**< Base 10dB attenuation code for Channel 5 */
#define PT2258_CMD_ATTEN_10DB_CH6   0xA0        /**< Base 10dB attenuation code for Channel 6 */

#define PT2258_CMD_ATTEN_1DB_ALL    0xE0        /**< Base 1dB attenuation code for ALL six channels simultaneously */
#define PT2258_CMD_ATTEN_1DB_CH1    0x90        /**< Base 1dB attenuation code for Channel 1 */
#define PT2258_CMD_ATTEN_1DB_CH2    0x50        /**< Base 1dB attenuation code for Channel 2 */
#define PT2258_CMD_ATTEN_1DB_CH3    0x10        /**< Base 1dB attenuation code for Channel 3 */
#define PT2258_CMD_ATTEN_1DB_CH4    0x30        /**< Base 1dB attenuation code for Channel 4 */
#define PT2258_CMD_ATTEN_1DB_CH5    0x70        /**< Base 1dB attenuation code for Channel 5 */
#define PT2258_CMD_ATTEN_1DB_CH6    0xB0        /**< Base 1dB attenuation code for Channel 6 */