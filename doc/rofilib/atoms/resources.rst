Embded Resources Management
===========================

There are some APIs (e.g., VTK) which can load resources (models, textures) only
from files specified by a name, not by a file handle or memory location. This
APIs prevent embedding the resources into the binary. Embedding the resources
into binary makes it portable and install-location independent.

:cpp:class:`ResourceManager` and :cpp:class:`ResourceFile` help to create a
temporary file from a resource embedded in the binary, and therefore, provide a
file name for such resource.

Usage
-----

To use an embedded resource you have to:

- embed the resource into the binary as a blob,
- load it in the program,
- get filename.

Embedding of the resources is handled by a CMake macro:

.. code-block:: cmake

    # resources can be an arbitrary, but unique name
    # multiple resource files are allowed
    add_resources(resources "resouceFile1.bmp" resourceFile2.obj")
    # Then you specify resources as one of the sources for you executable/library
    add_executable(myProject ${resources} main.cpp)

Then you can use the resource in your code:

.. code-block:: cpp

    // Slashes and dots in file names are replaced by an underscore
    // Note that the resource name is not string (it is a symbol actually)
    ResourceFile myResource = LOAD_RESOURCE_FILE( resouceFile1_bmp );
    // Call the api with file name:
    loadTexture( myResource.name() );


Classes reference
-----------------

.. doxygenclass:: ResourceManager
    :project: lib

.. doxygenclass:: ResourceFile
    :project: lib

Macro reference
---------------

.. doxygendefine:: LOAD_RESOURCE_FILE
    :project: lib