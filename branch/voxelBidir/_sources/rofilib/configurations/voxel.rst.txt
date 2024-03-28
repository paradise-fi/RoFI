Voxel Configuration
===================

.. seealso::
    The Voxel configuration is mainly developed in
    `Rust <https://www.rust-lang.org/>`_
    and as such has its own documentation.

Voxel configuration tries to minimize
the information needed to represent a world with RoFI modules.

For this reason there are some limitations in expresivity of Voxel
configuration, which allows for a more space efficient representation
and for more efficient algorithms when using this representation:

  - All modules have to be a Universal module.
  - All joint values have to be a multiple of 90 degrees.
  - Modules are not distinguishable.
  - Shoes of modules and rotations are not distinguishable
    if they can be mapped to each other.
  - The configuration is not fixed in space.

These limitations imply that symmetries are baked into the representation.

.. contents::

.. note::
    The name `Voxel <https://en.wikipedia.org/wiki/Voxel>`_
    comes from the original idea representing the world as a 3D grid
    of values that are either empty or contain a shoe of a RoFI module.

    Since then the representation of the voxel world has shifted and
    it is no longer stored as a 3D vector of values, but the meaning
    and name `Voxel` stayed the same.

Explicit Connections
--------------------

Voxel world allows specifying explicit connections,
but this is not yet reflected in cpp code.

Voxel Configuration in Cpp
--------------------------

The class representing Voxel configuration in cpp
is :cpp:class:`rofi::voxel::VoxelWorld`.
:cpp:class:`rofi::voxel::VoxelWorld` can be easily converted to and from
:cpp:class:`rofi::configuration::RofiWorld`
by :cpp:func:`rofi::voxel::VoxelWorld::toRofiWorld`
and :cpp:func:`rofi::voxel::VoxelWorld::fromRofiWorld` respectively.

.. note::
    When converting from :cpp:class:`rofi::voxel::VoxelWorld`
    to :cpp:class:`rofi::configuration::RofiWorld`,
    :cpp:func:`rofi::voxel::VoxelWorld::toRofiWorld` expects
    that the :cpp:class:`rofi::voxel::VoxelWorld` is connected.

    If you want to convert :cpp:class:`rofi::voxel::VoxelWorld` that
    isn't connected, you can set parameter `fixateModulesByOne`.

Types, Constants and Support Classes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. doxygenconstant:: rofi::voxel::precision
    :project: lib

.. doxygentypedef:: rofi::voxel::Position
    :project: lib

.. doxygenenum:: rofi::voxel::Axis
    :project: lib

.. doxygenenum:: rofi::voxel::JointPosition
    :project: lib

.. doxygenstruct:: rofi::voxel::Direction
    :project: lib

Classes
^^^^^^^

.. doxygenstruct:: rofi::voxel::Voxel
    :project: lib

.. doxygenstruct:: rofi::voxel::VoxelWorld
    :project: lib
