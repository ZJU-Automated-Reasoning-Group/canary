# Configuration file for the Sphinx documentation builder.

# -- Project information -----------------------------------------------------
project = 'Lotus'
copyright = '2025, ZJU Automated Reasoning Group'
author = 'ZJU Automated Reasoning Group'
release = '0.1'

# -- General configuration ---------------------------------------------------
extensions = []
templates_path = ['_templates']
exclude_patterns = []

# -- Options for HTML output -------------------------------------------------
html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
# html_logo = '../logo.jpg'
html_theme_options = {
    'logo_only': False,
    'display_version': True,
}

# -- Custom CSS to adjust logo size ------------------------------------------
html_css_files = ['custom.css']

# Make sure the custom.css file exists
import os
css_dir = os.path.join(os.path.dirname(__file__), '_static')
if not os.path.exists(css_dir):
    os.makedirs(css_dir)

css_file = os.path.join(css_dir, 'custom.css')
if not os.path.exists(css_file):
    with open(css_file, 'w') as f:
        f.write("""
/* Custom CSS for Lotus documentation */
.logo {
    width: 150px !important;
    height: auto !important;
}

/* Adjust sidebar logo container */
.wy-side-nav-search {
    padding-top: 10px;
}

/* Make the logo image in the sidebar smaller */
.wy-side-nav-search > a img.logo {
    max-width: 70%;
    height: auto !important;
}
""") 