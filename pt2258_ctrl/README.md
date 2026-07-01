[![Component Registry](https://components.espressif.com/components/romr/pt2258_ctrl/badge.svg)](https://components.espressif.com/components/romr/pt2258_ctrl)

# PT2258 Controller

An ESP-IDF volume management engine for the PT2258 IC, featuring logarithmic scaling, channel masking, per-channel balance offsets, and internal state caching.

## Features

- **Master volume control**: 0-100% volume with logarithmic LUT or linear (optional) scaling
- **Volume stepping**: Increment/decrement volume by percentage steps
- **Per-channel offsets**: Adjust individual channel levels relative to master volume
- **Global mute**: Chip-level mute control for all channels
- **Offset limits**: Configurable range for channel offsets
- **State tracking**: Maintains internal state for volume, offsets, and mute status
- **Channel Masking**: Initialize only the channels you actually wired on your PCB layout.

## Dependencies & Integration

This component acts as a high-level software engine and **strictly requires** the low-level hardware driver `romr/pt2258` component to communicate with the chip.

> [!NOTE]
> **Automatic Resolution**
> The core `pt2258` driver is already declared as a direct dependency inside this component's `idf_component.yml`. When you add `pt2258_ctrl` to your project, the ESP-IDF Component Manager will **automatically fetch and install both components**.

You only need to add the controller to your project's dependencies:

```bash
idf.py add-dependency romr/pt2258_ctrl
```

or manually add to your project's `idf_component.yml`:

```yaml
dependencies:
  romr/pt2258_ctrl: "*" # Automatically resolves and installs romr/pt2258
```

## Usage example
Because the core driver is pulled automatically, you can directly include both interfaces (pt2258.h and pt2258_ctrl.h) in your application code:

> [!NOTE]
> **Thread Safety**
>
> This component is not thread-safe. Use from a single task or add external synchronization if accessing from multiple contexts.

```c
#include "esp_log.h"
#include "i2c_bus.h"
#include "pt2258.h"
#include "pt2258_ctrl.h"

static const char *TAG = "main";

void app_main(void)
{
    // 1. Initialize your I2C master bus and add PT2258 device (see pt2258 documentation for example)
    
    // ... your I2C bus and device initialization code ...

    // 2. Configure PT2258 controller
    // Enable channels 1, 2, and 3 (stereo setup)
    pt2258_ctrl_config_t ctrl_cfg = {
        .pt2258 = pt2258_handle,
        .active_channels_mask = PT2258_CH1_ENABLE | PT2258_CH2_ENABLE | PT2258_CH3_ENABLE,
        .init_volume = 50,              // Start at 50% volume
        .offset_limits = 20,            // Allow +/- 20 dB offset per channel (must be <= PT2258_MAX_ATTENUATION)
        .use_linear_volume = false,     // Use logarithmic volume scaling (false = LUT, true = linear)
        .init_mute = true,              // Start with audio disabled
    };

    pt2258_ctrl_handle_t ctrl_handle = NULL;
    esp_err_t ret = pt2258_ctrl_create(&ctrl_cfg, &ctrl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PT2258 controller");
        pt2258_delete(&pt2258_handle);
        return;
    }

    // 3. Control volume and offsets example
    // Boost channel 1 by +5 dB, reduce channel 2 by -3 dB
    pt2258_ctrl_set_offset(ctrl_handle, 1, 5);
    pt2258_ctrl_set_offset(ctrl_handle, 2, -3);

    // Set master volume to 70%
    pt2258_ctrl_set_master_volume(ctrl_handle, 70);

    // Unmute to enable audio output
    pt2258_ctrl_set_mute(ctrl_handle, false);

    // Increase volume by 10%
    pt2258_ctrl_volume_step_up(ctrl_handle, 10);
    
    // Decrease volume by 5%
    pt2258_ctrl_volume_step_down(ctrl_handle, 5);

    // Get current state
    uint8_t current_volume;
    pt2258_ctrl_get_master_volume(ctrl_handle, &current_volume);
    ESP_LOGI(TAG, "Current volume: %d%%", current_volume);

    int16_t ch1_offset;
    pt2258_ctrl_get_offset(ctrl_handle, 1, &ch1_offset);
    ESP_LOGI(TAG, "Channel 1 offset: %d dB", ch1_offset);

    uint8_t ch1_attenuation;
    pt2258_ctrl_get_attenuation(ctrl_handle, 1, &ch1_attenuation);
    ESP_LOGI(TAG, "Channel 1 attenuation: %d dB", ch1_attenuation);

    // ... your application logic ...

    // 4. Clean up resources when done
    pt2258_ctrl_delete(&ctrl_handle);
    pt2258_delete(&pt2258_handle);
}
```