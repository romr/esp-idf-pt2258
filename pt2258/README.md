[![Component Registry](https://components.espressif.com/components/romr/pt2258/badge.svg)](https://components.espressif.com/components/romr/pt2258)

# PT2258

ESP-IDF component driver for the **PT2258** 6-channel electronic volume controller with I2C interface. This component provides a fully decoupled interface to interact with the PT2258 using a user-defined I2C write callback. This architecture ensures out-of-the-box compatibility with any I2C bus implementation — including the native **ESP-IDF** drivers, the distinct `i2c_bus` implementations from **ESP-ADF** and **ESP-IoT-Solution**, or completely custom wrappers — effectively eliminating framework and driver conflicts in audio projects.

## Features

- **6-channel volume control**: Individual attenuation control for each channel plus master volume.
- **Fine-grained attenuation**: 0 to 79 dB in 1 dB steps.
- **Global mute**: Chip-level mute control for all channels.
- **Multiple I2C addresses**: Up to 4 configurable addresses via hardware pins.
- **Transport Independent**: Easily integrates into any project by injecting your own I2C write function.

## Hardware Specifications & Timing

For details, see the [PT2258 datasheet](https://www.alldatasheet.com/datasheet-pdf/view/201161/PTC/PT2258.html).

### I2C Addressing

The PT2258 supports up to 4 selectable I2C addresses via the hardware configuration of the `CODE1` (Pin 17) and `CODE2` (Pin 4) pins. This allows you to daisy-chain up to 4 chips on a single I2C bus.

| CODE1 (Pin 17) | CODE2 (Pin 4) | 8-bit Address (Datasheet) | C Macro Definition (8-bit) | 7-bit Address | C Macro Definition (7-bit) |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **GND (0)** | **GND (0)** | 0x80 | `PT2258_I2C_ADDR_0_8BIT` | 0x40 | `PT2258_I2C_ADDR_0` |
| **GND (0)** | **VCC (1)** | 0x84 | `PT2258_I2C_ADDR_1_8BIT` | 0x42 | `PT2258_I2C_ADDR_1` |
| **VCC (1)** | **GND (0)** | 0x88 | `PT2258_I2C_ADDR_2_8BIT` | 0x44 | `PT2258_I2C_ADDR_2` |
| **VCC (1)** | **VCC (1)** | 0x8C | `PT2258_I2C_ADDR_3_8BIT` | 0x46 | `PT2258_I2C_ADDR_3` |

### Critical Hardware Considerations

> [!IMPORTANT]
> **Power-On Stabilization Delay** 
> After power-up, the PT2258 requires a stabilization period. You **must wait at least 200 ms** before transmitting any I2C signals. Initiating communication early can lock up the chip's internal logic, requiring a hard power cycle.

> [!WARNING]
> **Uninitialized Register Silence** 
> The PT2258 does not load default volume values on boot. While `pt2258_create()` automatically clears internal registers, you must explicitly set an attenuation value for each channel. Unconfigured channels will likely output no audio.

## Usage Example

This driver is I2C driver-independent and relies on Dependency Injection for communication. When initializing the driver, you must provide:

- An I2C Write Callback: A custom function matching the pt2258_write_cb_t signature that dictates how bytes are transmitted.
- A Bus Context / Device Handle: A pointer to your specific I2C device handle or transport configuration structure. The driver holds this pointer and passes it back to your callback function whenever an I2C operation is performed.

### ESP-IDF Native Driver

The following example demonstrates how to initialize the PT2258 using the native ESP-IDF v5 master driver:

```c
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "pt2258.h"

static const char *TAG = "main";

// Implementation of the I2C write callback matching the pt2258_write_cb_t signature
static esp_err_t pt2258_i2c_write_cb(void *handle, const uint8_t *data, size_t len)
{
    // Directly use the injected native ESP-IDF device handle
    return i2c_master_transmit((i2c_master_dev_handle_t)handle, data, len, -1);
}

void app_main(void)
{
    // 1. Initialize the I2C master bus
    i2c_master_bus_config_t bus_config = {
        // ... your configuration ...
    };
    i2c_master_bus_handle_t bus_handle;
    i2c_master_new_bus(&bus_config, &bus_handle);

    // 2. Add the PT2258 device to the I2C bus (using 7-bit address)
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PT2258_I2C_ADDR_0, // 0x40 (7-bit)
        .scl_speed_hz = 100000,              // 100 kHz Standard Mode
    };
    i2c_master_dev_handle_t pt2258_i2c_handle;
    i2c_master_bus_add_device(bus_handle, &dev_config, &pt2258_i2c_handle);

    // 3. Configure PT2258 driver settings by injecting the dependencies
    pt2258_config_t pt2258_cfg = {
        .write_cb = pt2258_i2c_write_cb,        // Inject the specific I2C write callback
        .i2c_dev_handle = pt2258_i2c_handle     // Inject native device handle directly as context
    };
    
    // Crucial: Wait at least 200ms after Power-ON to ensure PT2258 stability
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // 4. Create the PT2258 driver instance
    pt2258_handle_t pt2258_handle;
    esp_err_t err = pt2258_create(&pt2258_cfg, &pt2258_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create PT2258 driver: %s", esp_err_to_name(err));
        return;
    }
    
    // 5. Configure and control the PT2258
    // Mute channels before changing configurations to avoid audio pops
    pt2258_set_mute(pt2258_handle, true);

    // 6. Adjust volume (attenuation)
    // Set Master Volume (All channels) to -15 dB
    pt2258_set_attenuation(pt2258_handle, PT2258_CH_ALL, 15);

    // Set Channel 1 specifically to -40 dB
    pt2258_set_attenuation(pt2258_handle, PT2258_CH_1, 40);

    // Unmute the chip to let audio pass through
    pt2258_set_mute(pt2258_handle, false);

    // ... your application logic ...

    // 7. Clean up resources when done
    pt2258_delete(&pt2258_handle);
}
```

### ESP-ADF i2c_bus Driver

The following example demonstrates how to initialize the PT2258 using the ESP-ADF i2c_bus:

```c
// Define the I2C transport structure for the ESP-ADF i2c_bus, which holds the bus handle and device address
typedef struct {
    i2c_bus_handle_t bus_handle;
    uint8_t i2c_addr;
} pt2258_i2c_transport_t;

// Implementation of the I2C write callback matching the pt2258_write_cb_t signature
static esp_err_t pt2258_i2c_write_cb(void *handle, const uint8_t *data, size_t len)
{
    pt2258_i2c_transport_t *transport = (pt2258_i2c_transport_t *)handle;
    return i2c_bus_write_data(transport->bus_handle, transport->i2c_addr, (uint8_t *)data, len);
}

void app_main(void)
{
    // 1. Initialize the I2C bus
    i2c_config_t bus_config = {
        // ... your i2c configuration ...
    };
    i2c_bus_handle_t i2c_bus = i2c_bus_create(I2C_NUM_0, &bus_config);
 
    // 2. Create the I2C transport context instance
    static pt2258_i2c_transport_t pt2258_i2c_transport = {
        .bus_handle = i2c_bus,
        .i2c_addr = PT2258_I2C_ADDR_2_8BIT // 0x88 (8-bit)
    };

    // 3. Configure PT2258 driver by injecting transport dependencies
    pt2258_config_t pt2258_cfg = {
        .write_cb = pt2258_i2c_write_cb,        // Inject the specific I2C write callback
        .i2c_dev_handle = &pt2258_i2c_transport // Inject the transport structure as the device handle context
    };
    
    // Crucial: Wait at least 200ms after Power-ON to ensure PT2258 stability
    vTaskDelay(pdMS_TO_TICKS(200)); 
    
    // 4. Create the PT2258 driver instance
    pt2258_handle_t pt2258_handle;
    esp_err_t err = pt2258_create(&pt2258_cfg, &pt2258_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create PT2258 driver: %s", esp_err_to_name(err));
        return;
    }
    
    // 5. Configure and control the PT2258
    // Mute channels before changing configurations to avoid audio pops
    pt2258_set_mute(pt2258_handle, true);

    // Set All channels to -15 dB
    pt2258_set_attenuation(pt2258_handle, PT2258_CH_ALL, 15);

    // Set Channel 1 specifically to -40 dB
    pt2258_set_attenuation(pt2258_handle, PT2258_CH_1, 40);

    // Unmute the chip to let audio pass through
    pt2258_set_mute(pt2258_handle, false);

    // ... your application logic ...

    // 7. Clean up resources when done
    pt2258_delete(&pt2258_handle);
}
```
### Espressif IoT Solution i2c_bus

The following example demonstrates how to initialize the PT2258 using the Espressif IoT Solution i2c_bus:

```c
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"
#include "pt2258.h"

static const char *TAG = "main";

// Implementation of the I2C write callback matching the pt2258_write_cb_t signature
static esp_err_t pt2258_i2c_write_cb(void *handle, const uint8_t *data, size_t len)
{
    // Directly use the injected native ESP-IDF device handle
    return i2c_bus_write_data(handle, PT2258_I2C_ADDR_2_8BIT, (uint8_t *)data, len);
}

void app_main(void)
{
    // 1. Initialize the I2C bus
    const i2c_config_t bus_config = {
        // ... your i2c configuration ...
    };
    i2c_bus_handle_t i2c_bus = i2c_bus_create(I2C_NUM_0, &bus_config);
 
    // 2. Add the PT2258 device to the I2C bus (using 7-bit address)
    i2c_bus_device_handle_t pt2258_i2c_dev_handle = i2c_bus_device_create(i2c_bus, PT2258_I2C_ADDR_2, 0);

    // 3. Configure PT2258 driver by injecting transport dependencies
    pt2258_config_t pt2258_cfg = {
        .write_cb = pt2258_i2c_write_cb,            // Inject the specific I2C write callback
        .i2c_dev_handle = pt2258_i2c_dev_handle     // Inject the device handle directly
    };
    
    // Crucial: Wait at least 200ms after Power-ON to ensure PT2258 stability
    vTaskDelay(pdMS_TO_TICKS(200)); 
    
    // 4. Create the PT2258 driver instance
    pt2258_handle_t pt2258_handle;
    esp_err_t err = pt2258_create(&pt2258_cfg, &pt2258_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create PT2258 driver: %s", esp_err_to_name(err));
        return;
    }
    
    // 5. Configure and control the PT2258
    // Mute channels before changing configurations to avoid audio pops
    pt2258_set_mute(pt2258_handle, true);

    // Set All channels to -15 dB
    pt2258_set_attenuation(pt2258_handle, PT2258_CH_ALL, 15);

    // Set Channel 1 specifically to -40 dB
    pt2258_set_attenuation(pt2258_handle, PT2258_CH_1, 40);

    // Unmute the chip to let audio pass through
    pt2258_set_mute(pt2258_handle, false);

    // ... your application logic ...

    // 7. Clean up resources when done
    pt2258_delete(&pt2258_handle);
}
```