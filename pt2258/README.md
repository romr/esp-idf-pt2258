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
>
> After power-up, the PT2258 requires a stabilization period. You **must wait at least 200 ms** before transmitting any I2C signals. Initiating communication early can lock up the chip's internal logic, requiring a hard power cycle.

> [!WARNING]
> **Uninitialized Register Silence** 
>
> The PT2258 does not load default volume values on boot. While `pt2258_create()` automatically clears internal registers, you must explicitly set an attenuation value for each channel. Unconfigured channels will likely output no audio.

## Usage

> [!TIP]
> **Out-of-the-box Ecosystem Adapters**
>
> To avoid writing boilerplate callback functions manually, you can use one of the plug-and-play initialization adapters tailored for your specific workflow:
>
> - 🔌 **ESP-IDF v5 Native I2C Master Adapter** — [GitHub](https://github.com/romr/pt2258_esp_idf) · [ESP Registry](https://components.espressif.com/components/romr/pt2258_esp_idf)
> - 🔌 **ESP-ADF i2c_bus Adapter** — [GitHub](https://github.com/romr/pt2258_esp_adf) · [ESP Registry](https://components.espressif.com/components/romr/pt2258_esp_adf)
>
> If you are using these adapters, you can skip the manual dependency injection setup below.

This driver is I2C bus realization-independent and relies on Dependency Injection for communication. When initializing the driver, you must provide:

- **A Bus Context / Device Handle**: A pointer to your specific I2C device handle or transport configuration structure (`transport_ctx`). The driver holds this pointer and passes it back to your callback function whenever an I2C operation is performed.
- **An I2C Write Callback**: A custom function matching the `pt2258_write_cb_t` signature that dictates how bytes are transmitted.
- **An Optional Cleanup Callback**: A custom function matching the `pt2258_cleanup_cb_t` signature to free memory if the transport context was allocated dynamically.

### I2C Write Callback Function

The callback function must match the following signature:

```c
typedef esp_err_t (*pt2258_write_cb_t)(void *transport, const uint8_t *data, size_t len);
```

- `handle`: A pointer to your I2C device handle or transport configuration structure.
- `data`: A pointer to the data to be written.
- `len`: The length of the data to be written.
- Returns: `ESP_OK` on success, or an error code on failure.

Example for native ESP-IDF v5 I2C Bus master driver:

```c
static esp_err_t _i2c_write_cb(void *transport, const uint8_t *data, size_t len)
{
    // Directly use the ESP-IDF native I2C master driver to write data into the PT2258
    return i2c_master_transmit((i2c_master_dev_handle_t)transport, data, len, -1);
}
```

Example for Espressif-IoT-Solution i2c_bus driver:

```c
static esp_err_t _i2c_write_cb(void *transport, const uint8_t *data, size_t len)
{
    // Use the Espressif-IoT Solution I2C driver to write data into the PT2258
    return i2c_bus_write_data(transport, PT2258_I2C_ADDR_2_8BIT, (uint8_t *)data, len);
}
```

### Usage in ESP-ADF

If you're using the driver from ESP-ADF's embedded i2c_bus driver, in addition to providing the callback function, you must also provide a custom transport layer. The transport layer is a structure that contains the I2C bus handle and the I2C address. This structure is injected into the driver as the bus context during initialization.

```c
typedef struct {
    i2c_bus_handle_t bus_handle;    // I2C bus handle
    uint8_t i2c_addr;               // I2C address
} pt2258_i2c_transport_t;

static esp_err_t _i2c_write_cb(void *transport, const uint8_t *data, size_t len)
{
    pt2258_i2c_transport_t *transport = (pt2258_i2c_transport_t *)transport;

    // Function i2c_bus_write_data is provided by the ESP-ADF i2c_bus driver to write data to the I2C device
    return i2c_bus_write_data(transport->bus_handle, transport->i2c_addr, (uint8_t *)data, len);
}
```

### Initialization

After defining the callback function (and if needed, the transport structure), you need to initialize the PT2258 driver by injecting dependencies.

```c
// ... Your existing I2C bus initialization and device handle creation code ...

// If using ESP-ADF i2c_bus driver, instead of device handle, define the transport structure
#if USE_ESP_ADF_I2C_BUS
static pt2258_i2c_transport_t pt2258_i2c_dev_handle = {
    .bus_handle = &i2c_bus_handle,
    .i2c_addr = PT2258_I2C_ADDR_2_8BIT
};
#endif

// Configure PT2258 driver 
pt2258_config_t pt2258_cfg = {
    .transport_ctx = &pt2258_i2c_dev_handle,  // Inject the I2C device handle context or transport structure
    .write_cb = _i2c_write_cb,                // Inject the specific I2C write callback
};

// Crucial: Wait at least 200ms after Power-ON to ensure PT2258 stability
vTaskDelay(pdMS_TO_TICKS(200)); 
    
// 3. Create the PT2258 driver instance
pt2258_handle_t pt2258_handle;
esp_err_t err = pt2258_create(&pt2258_cfg, &pt2258_handle);
if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create PT2258 driver: %s", esp_err_to_name(err));
    return;
}
```

### Control

After the driver is created, you can control the PT2258 by calling API functions, for example:

```c
// Mute all channels
pt2258_set_mute(pt2258_handle, true);

// Set the attenuation for channel 1 to 10dB
pt2258_set_attenuation(pt2258_handle, PT2258_CH_1, 10);

// Set the attenuation for all channels to 22dB
pt2258_set_attenuation(pt2258_handle, PT2258_CH_ALL, 22);
```

See [API documentation](https://romr.github.io/esp-idf-pt2258/pages/pt2258.html#api-reference) for more details.

### Delete

To delete the PT2258 driver instance:

```c
pt2258_delete(&pt2258_handle);
```