import os

project = 'ESP-IDF-PT2258'
author = 'Roman Fedorov'
language = "en"

ga_id = os.environ.get('GA_ANALYTICS_ID')
github_url = os.environ.get('GITHUB_URL', None)
esp_registry_url = os.environ.get('ESP_REGISTRY_URL', None)

extensions = [
    'sphinx.ext.autodoc',
    'sphinx_copybutton',
    'myst_parser',
    'breathe',
    'sphinx_rtd_theme',
]

if ga_id:
    extensions.append('sphinxcontrib.googleanalytics')
    googleanalytics_id = ga_id
    googleanalytics_enabled = True

myst_enable_extensions = [
    "alert",
    "colon_fence",
    "deflist",
]
myst_heading_anchors = 3

breathe_projects = {
    "pt2258_docs": "_build/xml"
}
breathe_default_project = "pt2258_docs"

html_theme = 'sphinx_rtd_theme'
html_title = "ESP-IDF-PT2258"
html_show_sourcelink = False

html_theme_options = {
    'style_external_links': True,
    'version_selector': False,
    'language_selector': False,
    'collapse_navigation': False,
}

html_context = {
    'github_url': github_url,
    'esp_registry_url': esp_registry_url,
}

html_show_copyright = False
html_last_updated_fmt = '%b %d, %Y'

templates_path = ['_templates']

exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']