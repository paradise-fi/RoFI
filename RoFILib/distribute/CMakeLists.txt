cmake_minimum_required(VERSION 3.10)
project(rofi-distribute)
SET(CMAKE_CXX_COMPILER mpic++)
SET(CMAKE_CXX_FLAGS ${MPI_CXX_COMPILE_FLAGS})

include_directories(../reconfig)

set(SOURCES DistributedModuleProperties.h DistributedModule.cpp DistributedModule.h DistributedPrinter.h DistributedReader.h AlgorithmFullConfiguration.cpp AlgorithmFullConfiguration.h AlgorithmPartialConfiguration.cpp AlgorithmPartialConfiguration.h)

add_executable(rofi-distribute main.cpp ${SOURCES})

target_link_libraries(rofi-distribute ${ARMADILLO_LIBRARIES} ${VTK_LIBRARIES})