[![Component Registry](https://components.espressif.com/components/romr/pt2258_esp_idf/badge.svg)](https://components.espressif.com/components/romr/pt2258_esp_idf)

# PT2258 ESP-IDF Adapter

This component provides an ESP-IDF initialization adapter for the PT2258 6-channel electronic volume controller IC. It bridges the decoupled low-level PT2258 driver to the native ESP-IDF `i2c_master` transport layer.

## Add to your project

```bash
idf.py add-dependency romr/pt2258_esp_idf
```

## Usage

> [!IMPORTANT]
> **This component is an initialization adapter only**
>
> This adapter functions strictly as an initialization adapter and **automatically handles and installs** the core PT2258 I2C driver ([GitHub](https://github.com/romr/pt2258) · [ESP Registry](https://components.espressif.com/components/romr/pt2258)) as a dependency. There is no need for manual installation or low-level setup.
>
> This adapter does not provide any runtime functionality. All subsequent runtime operations — such as volume attenuation adjustment and mute toggling — are performed using the native API functions defined in the pt2258 I2C driver component (see the [docs](https://romr.github.io/esp-idf-pt2258/pages/pt2258.html)).

### Initialization

```c
// 1. Initialize I2C bus

// ... Your I2C bus config and initialization code here ...

// 2. Initialize PT2258 with I2C bus
pt2258_esp_idf_config_t pt2258_config = {
    .bus_handle = i2c_master_bus_handle,
    .i2c_addr = PT2258_I2C_ADDR_2,          // refer to pt2258 I2C driver docs for address options
    .clk_speed = 100000,                    // I2C SCL frequency in Hz
};
pt2258_handle_t pt2258;
esp_err_t ret = pt2258_esp_idf_create(&pt2258_config, &pt2258);
ESP_ERROR_CHECK(ret);

// 3. Then you can use the pt2258 driver functions for volume control
// See pt2258 I2C driver documentation for available functions
pt2258_set_mute(pt2258, true);
```