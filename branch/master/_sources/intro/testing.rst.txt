Writing and running tests
=========================

The project features a number of unit tests. If you develop a new part of the
project, your are encouraged to write them. Not only it makes your life easier
(as it is much easier to code and debug against tests), but you also ensure that
nobody will break your code in the future without knowing about it.

All tests are run on every commit and pull request on GitHub automatically. If
the tests fail, you will be notified via email.

Running the test
----------------

You can easily run the test via ``rmake --test <argument>``. You can specify the
following arguments:

.. code-block::

    rmake --test --all               Compile and run all configured tests
    rmake --test <testTargetName>... Compile and run test for target
    rmake --test <pathName>...       Compile and run tests from given path prefix
    rmake --test <suitename>...      Compile and run tests from given suite


The command will compile and run specified tests. Also, the tab-completion shows
you the available options. Note that only tests from configured suites are
available.

You can specify the test based on the same criterion as `compilation targets
<compiling.html#compilation-targets>`__. So you can run tests for the whole
suite, given a test or all tests in the specified path prefix. For the sake of
simplicity, you currently cannot pass any arguments to the tests. If you need
to do so, you can always invoke the test executable directly.

Writing tests
-------------

For the purposes of RoFI, a test is an executable in ``PATH`` that satisfies the
following:

-  its name starts with the prefix `test-`,
-  it has an associated CMake target in one of the projects,
-  it can run with no command line arguments,
-  if and only if the tests fails, the return code is non-zero, and
-  it gives user-readable output on the stdout/stderr indicating what went wrong
   in the case of failure.

Usually, a test will a be `Catch2 <https://github.com/catchorg/Catch2>`__ binary
registered in CMake. But it can also be e.g., a shell script or Python script
that setups much more complex tests. In case of hardware, it can be a script
that flashes the target microcontroller with a test binary and captures the
output on a debug serial line.

Writing test for C++
--------------------

As most of the components for RoFI are written in C++, we provide a simple guide
for writing C++ tests. We use `Catch2 <https://github.com/catchorg/Catch2>`__ as
the primary testing framework. You should use it if don't have a particular
reason to not use it.

Most of the compilation suites already provide it, so simply include it in your
CMake like this:

.. code-block::cmake

    file(GLOB TEST_SRC test/*.cpp)
    add_executable(test-coolSoftwareModule ${TEST_SRC})
    target_link_libraries(test-coolSoftwareModule PRIVATE Catch2::Catch2)

And that's everything you need. Now you can just write test source files in the
`test` directory. See `Catch2 tutorial
<https://github.com/catchorg/Catch2/blob/v2.x/docs/tutorial.md>`__ for more
details.

If your software module is not header-only, it makes sense to extract it into a
static library and link that library to your executable and tests. It will
significantly speed-up the compilation as you don't have to compile all sources
twice.
