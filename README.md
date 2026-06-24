# ESP-IDF-PT2258

An ESP-IDF audio control implementation for the **PT2258 6-Channel Electronic Volume Controller IC**.

[PT2258 Datasheet](https://www.alldatasheet.com/datasheet-pdf/view/201161/PTC/PT2258.html)

## Repository Structure

### 📂 [pt2258 (I2C Driver)](https://github.com/romr/esp-idf-pt2258/tree/main/pt2258)
Provides direct, stateless hardware control.

## Adding to Your Project

### Option 1: ESP Component Manager (Recommended)
You can add components directly to your project dependencies using the IDF Component Manager:

```bash
idf.py add-dependency romr/pt2258
```

### Option 2: Git Submodule
Add this entire repository into your project's components/ directory.

```bash
cd <your_project_root>
git submodule add https://github.com/romr/esp-idf-pt2258.git components/esp-idf-pt2258
```