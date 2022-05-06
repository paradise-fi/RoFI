Documentation Suite
===================

This suite builds this documentation. The documentation is build using Sphinx
and Breathe.

Dependencies
------------

To build the documentation, Sphinx, Breathe, Sphinx-RTD-Theme, MyST,
Doxygen and Graphviz are required.

On APT based system, these dependencies can be installed via:

.. code-block:: sh

    $ apt-get install doxygen graphviz
    $ pip3 install sphinx breathe myst_parser sphinx-rtd-theme


If you intend to use `rmake --doc` command, you need to install additional package

.. code-block:: sh

    $ apt-get install inotify-tools

Build options
-------------

There are no build options.
