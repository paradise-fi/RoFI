Parsing Configurations in C++
=============================

When parsing configurations in C++ tools, the configurations get usually
converted into :cpp:class:`rofi::configuration::RofiWorld`.
For this, tools can use the parsing function
:cpp:func:`rofi::parsing::parseRofiWorld`
or :cpp:func:`rofi::parsing::parseRofiWorldSeq`.

For the other conversion, tools can use
:cpp:func:`rofi::parsing::writeRofiWorld`
or :cpp:func:`rofi::parsing::writeRofiWorldSeq`.

.. note::
    When parsing you may want to check whether the configuration is empty
    or whether the configuration is fixed in space.

    If you want to allow for not fixed configurations and be able to prepare
    them (without regards to the real position), you can use
    :cpp:func:`rofi::parsing::fixateRofiWorld`.

    Also don't forget to validate the configuration if needed.

Helper Functions
----------------

.. doxygenfunction:: rofi::parsing::parseJson
    :project: lib

.. doxygenfunction:: rofi::parsing::getFromJson
    :project: lib

Helper RofiWorld Functions
--------------------------

.. doxygenfunction:: rofi::parsing::fixateRofiWorld
    :project: lib

.. doxygenfunction:: rofi::parsing::toSharedAndValidate
    :project: lib

.. doxygenfunction:: rofi::parsing::validateRofiWorldSeq
    :project: lib

.. doxygenfunction:: rofi::parsing::validatedRofiWorldSeq
    :project: lib

Conversion Functions
--------------------

.. doxygenfunction:: rofi::parsing::getRofiWorldFromJson
    :project: lib

.. doxygenfunction:: rofi::parsing::getRofiWorldSeqFromJson
    :project: lib

.. doxygenfunction:: rofi::parsing::convertRofiWorldSeqToJson
    :project: lib

.. doxygenfunction:: rofi::parsing::convertFromRofiWorldSeq
    :project: lib

.. doxygenfunction:: rofi::parsing::convertToRofiWorldSeq
    :project: lib

.. doxygenfunction:: rofi::parsing::parseOldCfgFormat
    :project: lib

Multi-Format Conversions
------------------------

.. doxygenenum:: rofi::parsing::RofiWorldFormat
    :project: lib

.. doxygenfunction:: rofi::parsing::parseRofiWorld
    :project: lib

.. doxygenfunction:: rofi::parsing::writeRofiWorld
    :project: lib

.. doxygenfunction:: rofi::parsing::parseRofiWorldSeq
    :project: lib

.. doxygenfunction:: rofi::parsing::writeRofiWorldSeq
    :project: lib
