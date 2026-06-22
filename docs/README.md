# Documentation Guide (Doxygen + Sphinx)

This directory contains the configuration and source files required to automatically generate documentation for the **ESP-IDF-PT2258** components.

The documentation system uses a hybrid **Doxygen + Sphinx + Breathe** toolchain and is automatically deployed to GitHub Pages via CI/CD.

## Architecture Overview

The generation pipeline works as follows:
1. **Doxygen** parses the C header files (`.h`) and extracts structures, enums, macros, and function definitions into intermediate **XML** files.
2. **Breathe** acts as a bridge plugin, allowing Sphinx to read and render the Doxygen XML data.
3. **Sphinx** processes the XML blocks along with standard Markdown (`.md`) guides using the `MyST-Parser` and compiles everything into a responsive, unified static HTML website styled with `sphinx_rtd_theme`.

## Local Environment Setup

### Prerequisites
Before building the documentation locally, ensure you have the following installed on your machine:
* **Doxygen** (Ensure it is added to your system `PATH`)
* **Python 3.8+**

### Installation
Navigate to the `docs` directory and install the required Python packages using the provided `requirements.txt`:

```bash
cd docs
pip install -r requirements.txt
```

## How to Build Locally

### Manual Generation
Run the following commands sequentially from the `docs` directory:

```powershell
# 1. Extract API structure to XML via Doxygen
doxygen Doxyfile

# 2. Build the final HTML website via Sphinx
python -m sphinx -b html . _build/html
```
Once the build completes successfully, open `docs/_build/html/index.html` in your web browser to preview the documentation.

### Verifying Header Links (Optional)
By default, the custom repository links in the breadcrumbs area remain hidden during local builds to prevent clutter. To test their rendering locally, set the environment variables in your terminal session before compiling:

```powershell
# For PowerShell:
$env:GITHUB_URL="https://github.com/<YOUR_GITHUB_USERNAME>/<YOUR_REPOSITORY_NAME>"
$env:ESP_REGISTRY_URL="https://components.espressif.com/"

# Re-run the Sphinx build
python -m sphinx -b html . _build/html
```

### VS Code Task
If you are using the pre-configured workspace, you can simply press `Ctrl+Shift+B` to run the automated build task defined in `.vscode/tasks.json`.

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build Documentation (ESP-IDF PowerShell)",
            "type": "shell",
            "command": "& '<PATH_TO_YOUR_ESP_IDF_POWERSHELL_PROFILE_PS1>'; doxygen Doxyfile; if ($?) { sphinx-build -b html . _build/html }",
            "options": {
                "env": {
                    "GA_ANALYTICS_ID": "<YOUR_GA_TRACKING_ID>",
                    "GITHUB_URL": "https://github.com/<YOUR_GITHUB_USERNAME>/<YOUR_REPOSITORY_NAME>",
                    "ESP_REGISTRY_URL": "https://components.espressif.com"
                },
                "cwd": "${workspaceFolder}/docs",
                "shell": {
                    "executable": "powershell.exe",
                    "args": ["-NoProfile", "-ExecutionPolicy", "Bypass", "-Command"]
                }
            },
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": []
        }
    ]
}
```

## CI/CD Pipeline Deployment

The documentation is automatically built and securely deployed on every push to the main branch via GitHub Actions (`.github/workflows/docs-generate.yml`).

### Repository Configuration Requirements
For the deployment pipeline to function correctly, make sure the following settings are configured in your GitHub repository interface:

* **GitHub Pages Source:** Navigate to `Settings -> Pages` and change the **Source** under *Build and deployment* to **GitHub Actions**.
* **Secrets:** Under `Settings -> Secrets and variables -> Actions -> Secrets`, add:
  * `GA_ANALYTICS_ID`: Your Google Analytics 4 measurement ID (e.g., `G-XXXXXXXXXX`).
* **Variables:** Under `Settings -> Secrets and variables -> Actions -> Variables`, add:
  * `DOCS_GITHUB_URL`: The public HTTPS URL of this repository (`https://github.com/<YOUR_GITHUB_USERNAME>/<YOUR_REPOSITORY_NAME>`).
  * `DOCS_ESP_REGISTRY_URL`: The URL of your component in the Espressif Component Registry.