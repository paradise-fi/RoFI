Configuration
=============

Here you are supposed to say the general idea behind the configuration
representation and also list its capabilities. (e.g., you create and instance,
put modules, then you have to compute the matrix, etc.)

Feel free to add code examples.


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

.. doxygenclass:: rofi::configuration::Rofibot
    :project: configuration

.. doxygenclass:: rofi::configuration::Module
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


.. doxygenclass:: rofi::configuration::NoColision
    :project: configuration

.. doxygenclass:: rofi::configuration::SimpleCollision
    :project: configuration

Modules
-------

.. doxygenclass:: rofi::configuration::Pad
    :project: configuration

.. doxygenclass:: rofi::configuration::UniversalModule
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

Serialization
-------------

Add some text about the json configuration options.
(And options for different module types.)

Ideally also include the grammar for configuration.

.. doxygenfunction:: rofi::configuration::serialization::toJSON( const Rofibot& bot, Callback attrCb )
    :project: configuration

.. doxygenfunction:: rofi::configuration::serialization::toJSON( const Rofibot& bot )
    :project: configuration

.. doxygenfunction:: rofi::configuration::serialization::fromJSON( const nlohmann::json& j, Callback attrCb )
    :project: configuration

.. doxygenfunction:: rofi::configuration::serialization::fromJSON( const nlohmann::json& j )
    :project: configuration
