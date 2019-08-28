include(FindPackageHandleStandardArgs)

find_library(Z3_LIBRARIES NAMES z3 libz3 ltz3 libz3 lz3)
find_path(Z3_INCLUDE_DIRS z3.h z3++.h)

find_package_handle_standard_args(Z3 DEFAULT_MSG Z3_INCLUDE_DIRS Z3_LIBRARIES)
