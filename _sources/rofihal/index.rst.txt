RoFI HAL
========

RoFI Hardware Abstraction Layer, RoFI HAL for short, allows to use the same
code for both the physical RoFIs and for the simulated ones.


Types and Constants
-------------------

.. doxygenenum:: rofi::hal::ConnectorPosition
    :project: hal

.. doxygenenum:: rofi::hal::ConnectorOrientation
    :project: hal

.. doxygenenum:: rofi::hal::ConnectorLine
    :project: hal

.. doxygenenum:: rofi::hal::ConnectorEvent
    :project: hal


.. doxygenstruct:: rofi::hal::ConnectorState
    :project: hal


Classes
-------

.. doxygenclass:: rofi::hal::RoFI
    :project: hal

.. doxygenclass:: rofi::hal::Joint
   :project: hal

.. doxygenclass:: rofi::hal::Connector
    :project: hal


Networking
----------

.. doxygenclass:: rofi::hal::PBuf
    :project: lib
