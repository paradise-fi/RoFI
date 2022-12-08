Release engineering notes
=========================

This document describes several important notes about release engineering of
RoFI.

Continuous integration
----------------------

The projects uses GitHub actions as CI backend. The build process is described
by the file ``.github/workflows/build.yml``. The build tries to build every
suite and if present, run suite tests.

To save building and pulling build dependencies every time, the individual
stages run in Docker container. The images are hosted by ``gcr.io`` and every
member of the organization `paradise-fi <https://github.com/paradise-fi>`_ can
access it. Note that you need to invoke ``docker login`` to get access to
``gcr.io``. You can read more in `GitHub
documentation <https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-container-registry>`_.

The images are defined by the files ``Dockerfile`` and ``Dockerfile.doc``. The
images are shared across all branches (in order to save space). The images can
also be automatically updated via invoking ``releng/tools/updateDockerImages``.

Documentation
-------------

This documentation is build under the suite doc. The build process is driven by
the suite. Under the hood it uses Doxygen and Spinx.

In order to build per-branch documentation (available under
``https://paradise-fi.github.io/RoFI/branch/<git_branch_name>``) the master
branch has an extra build step which checks out all branches pushed to origin
and builds them. For security reasons, builds cannot trigger other builds,
therefore the master branch is built every night in order to ensure the
documentation is up-to-date.

