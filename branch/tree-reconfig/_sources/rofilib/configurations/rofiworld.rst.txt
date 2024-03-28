RoFI Configuration
==================

Configuration is a way of representing any collection of RoFI Modules
in an universal manner.
It contains all building blocks of the RoFI platform such as various modules
and joints.

Using these blocks, you can specify every RoFI world -- a set of rofibots
(connected set of RoFI modules fixed in space) -- and work with it.
Configuration computes positions of every component and provides
useful functions for manipulation with the whole world, including
some validity checks (e.g. collision checks).

.. contents::

Usage
-----

The main class is :cpp:class:`RofiWorld <rofi::configuration::RofiWorld>`
for which you create an instance
and add all the modules you want to have in the world.
You have to connect these modules together appropriately
to form individual rofibots.
If you want to use absolute positions, you have to fix
one of the components of each rofibot in space,
otherwise, the configuration cannot figure out its coordinates.

Example
-------

Let us create a world with a single bot consisted of two universal modules.

.. code-block:: cpp

    #include <configuration/rofiworld.hpp>

    // ...

    RofiWorld world;
    // add universal module with id 42 in the default state
    auto& m1 = world.insert( UniversalModule( 42, 0_deg, 0_deg, 0_deg ) );
    // add universal module with id 42 with beta set to 45 degrees and gamma to 90 degrees
    auto& m2 = world.insert( UniversalModule( 66, 0_deg, 45_deg, 90_deg ) );

    // connect A+X of the universal module with id = 42 to A-X of UM with id = 66
    connect( m1.connectors()[ 2 ], m2.connectors()[ 0 ], Orientation::North );
    // fix the position of the `shoe A` in { 0, 0, 0 }
    connect< RigidJoint >( m1.bodies()[ 0 ], { 0, 0, 0 }, identity );

With the `world` in hand, you can then prepare it (i.e. compute positions of each
part -- rofibot -- of the world) and check for its validity.

.. code-block:: cpp

    if ( auto prepared = world.prepare(); !prepared )
        std::cerr << "Could not prepare configuration: " << prepared.assume_error() << "\n";
    if ( auto valid = world.isValid( SimpleCollision() ); !valid )
        std::cerr << "Invalid configuration: " << valid.assume_error() << "\n";

    // or you can shorten the above to
    auto ok = world.validate( SimpleCollision() );

    // also, the SimpleCollision model is the default one, so you can ommit it too and get
    auto ok = world.validate();


Types and Constants
-------------------

.. doxygentypedef:: rofi::configuration::ModuleId
    :project: configuration

.. doxygenenum:: rofi::configuration::ComponentType
    :project: configuration

.. doxygenenum:: rofi::configuration::ModuleType
    :project: configuration

.. doxygenenum:: rofi::configuration::roficom::Orientation
    :project: configuration

Classes
-------

.. doxygenclass:: rofi::configuration::RofiWorld
    :project: configuration

.. doxygenclass:: rofi::configuration::Module
    :project: configuration

.. doxygenstruct:: rofi::configuration::Component
    :project: configuration

.. doxygenstruct:: rofi::configuration::Joint
    :project: configuration

.. doxygenstruct:: rofi::configuration::RigidJoint
    :project: configuration

.. doxygenstruct:: rofi::configuration::RotationJoint
    :project: configuration

.. doxygenstruct:: rofi::configuration::RoficomJoint
    :project: configuration

.. doxygenstruct:: rofi::configuration::ComponentJoint
    :project: configuration

.. doxygenstruct:: rofi::configuration::SpaceJoint
    :project: configuration


.. doxygenclass:: rofi::configuration::NoCollision
    :project: configuration

.. doxygenclass:: rofi::configuration::SimpleCollision
    :project: configuration

Module Classes
--------------

.. doxygenclass:: rofi::configuration::UniversalModule
    :project: configuration

.. doxygenclass:: rofi::configuration::Pad
    :project: configuration

.. doxygenclass:: rofi::configuration::UnknownModule
    :project: configuration

Functions
---------

.. doxygenfunction:: rofi::configuration::connect(const Component &c1, const Component &c2, roficom::Orientation o)
    :project: configuration

.. doxygenfunction:: rofi::configuration::connect(const Component &c, Vector refpoint, Args&&... args)
    :project: configuration

.. doxygenfunction:: rofi::configuration::makeComponentJoint
    :project: configuration
