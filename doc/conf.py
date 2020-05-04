# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))

import textwrap


# -- Project information -----------------------------------------------------

project = 'The RoFI Platform'
copyright = '2020, Paradise'
author = 'Paradise'


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'breathe',
    'recommonmark'
]
breathe_sources = {
    "driver": "../universalModule/software/RoFIDriver/",
    "roficom": "../RoFICoM/software/control_board/",
    "lib": "../RoFILib/",
    "gazebosim": "../simulator/",
}
breathe_projects = { name: "build/doxygen/" + name + "/xml" \
    for name in breathe_sources.keys() }
breathe_default_members = ('members', 'undoc-members')

# Tell sphinx what the primary language being documented is.
primary_domain = 'cpp'

# Tell sphinx what the pygments highlight language should be.
highlight_language = 'cpp'

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

exclude_patterns = ['readme.md']

if __name__ == "__main__":
    import textwrap
    import os
    # Generate makefile
    print(textwrap.dedent(
        """
        SPHINXOPTS    ?=
        SPHINXBUILD   ?= sphinx-build
        DOC_DIR       = .
        BUILDDIR      = build
        """
    ))

    for name, path in breathe_sources.items():
        print("{}_DEPS = $(realpath $(shell find {} -type f -not -path '*build*'))".format(name, path))

    print(textwrap.dedent(
        """
        .PHONY: all clean

        all: sphinx

        build:
        \tmkdir build

        sphinx: Makefile {doxygens}
        \t@$(SPHINXBUILD) -M html "$(DOC_DIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

        clean:
        \t@$(SPHINXBUILD) -M clean "$(DOC_DIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
        \trm -rf "$(BUILDDIR)"
        """
    ).format(doxygens = " ".join(breathe_projects.values())))

    for name, path in breathe_projects.items():
        print("{}: $({}_DEPS)".format(path, name))
        print("\t./extractDoxygen.sh {} {} {}".format(name, breathe_sources[name],
            os.path.dirname(path)))