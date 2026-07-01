# ESP-IDF-PT2258

An ESP-IDF audio control implementation for the **PT2258 6-Channel Electronic Volume Controller IC**.

[PT2258 Datasheet](https://www.alldatasheet.com/datasheet-pdf/view/201161/PTC/PT2258.html)

## Repository Structure

### 📂 [pt2258 (I2C Driver)](https://github.com/romr/esp-idf-pt2258/tree/main/pt2258)
Provides direct, stateless hardware control.

### 📂 [pt2258_ctrl (Controller)](https://github.com/romr/esp-idf-pt2258/tree/main/pt2258_ctrl)
Provides a controller for the PT2258 driver.

### 📂 [pt2258_esp_adf (ESP-ADF Adapter)](https://github.com/romr/esp-idf-pt2258/tree/main/pt2258_esp_adf)
ESP-ADF initialization adapter that bridges the PT2258 driver to the native ESP-ADF `i2c_bus` transport layer. Automatically handles installation of the core PT2258 I2C driver as a dependency.

### 📂 [pt2258_esp_idf (ESP-IDF Adapter)](https://github.com/romr/esp-idf-pt2258/tree/main/pt2258_esp_idf)
ESP-IDF initialization adapter that bridges the PT2258 driver to the native ESP-IDF `i2c_master` transport layer. Automatically handles installation of the core PT2258 I2C driver as a dependency.

## Adding to Your Project

### Option 1: ESP Component Manager (Recommended)
Depending on your project's framework, run the appropriate command in your project directory to add the component:

```bash
# If using the native ESP-IDF v5+ New I2C Master driver:
idf.py add-dependency "romr/pt2258_esp_idf"

# If using the PT2258 Controller:
idf.py add-dependency "romr/pt2258_ctrl"

# If using the ESP-ADF audio framework i2c_bus:
idf.py add-dependency "romr/pt2258_esp_adf"

# Only if implementing a completely custom transport layer or manual callbacks:
idf.py add-dependency "romr/pt2258"
```

> [!NOTE]
> **Automatic Dependency Resolution**
>
> When using any of the official adapters (pt2258_esp_idf or pt2258_esp_adf), the core romr/pt2258 driver is automatically resolved and installed as a required dependency by the ESP Component Manager. You do not need to add it manually.
>
> Direct installation of the core romr/pt2258 component is strictly reserved for projects requiring a custom, user-defined I2C transport implementation.

### Option 2: Git Submodule
Add this entire repository into your project's components/ directory.

```bash
cd <your_project_root>
git submodule add https://github.com/romr/esp-idf-pt2258.git components/esp-idf-pt2258
```