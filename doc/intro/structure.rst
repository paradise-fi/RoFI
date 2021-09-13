Project Structure
=================

The project is organized as a mono-repository, that is all the sources for the
whole project are in a single `Git repository
<https://github.com/paradise-fi/RoFI>`__.

There are the following directories:

-  ``hardwareModules``: All the hardware modules. CAD models, PCBs and related
   firmware.

    -  ``RoFICoM``: All sources for the mechanical connnector

    -  ``universalModule``: All sources for the universal module (does not
       contain concrete RoFI applications)
-  ``softwareComponents``: All libraries provided by the project. We do not
   distinguish whether the libraries target embedded or desktop. Each directory
   is single library and has its readme file containing further details. Some of
   the notable libraries are:

    -  ``atoms``: A library of general-purpose data structures and algorithms.
       This library has a :doc:`dedicated section <../rofilib/atoms/index>` of
       the documentation.
    -  ``configuration``: A library for representation of a RoFIBot. See
       :doc:`documentation <../rofilib/configuration>`.
    -  ``rofiHalSim`` and ``rofiHalPhys``: The hardware abstraction layer for
       RoFI modules.
-  ``tools``: The tools for working with RoFI
-  ``examples``: A self-contained, easily executable examples demonstrating the
   the project. Often referred from this documentation.
-  ``applications``: RoFI applications (case-studies, programs for the modules)
-  ``data``: All resources (simulator models, library of configurations)
-  ``media``: Pictures & videos of the project used in readmes.
-  ``releng``: Packaging & compilation
-  ``doc``: This documentation
