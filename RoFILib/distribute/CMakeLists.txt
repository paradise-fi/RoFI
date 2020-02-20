cmake_minimum_required(VERSION 3.6)
project(rofi-distribute)
set(CMAKE_CXX_COMPILER mpic++)
set(CMAKE_CXX_FLAGS ${MPI_CXX_COMPILE_FLAGS})

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})


set(SOURCES DistributedModuleProperties.h DistributedModule.cpp DistributedModule.h DistributedPrinter.h DistributedReader.h AlgorithmFullConfiguration.cpp AlgorithmFullConfiguration.h AlgorithmPartialConfiguration.cpp AlgorithmPartialConfiguration.h)

add_executable(rofi-distribute main.cpp ${SOURCES})
target_link_libraries(rofi-distribute reconfig ${VTK_LIBRARIES})
target_include_directories(rofi-distribute PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../extlib/)


add_executable(rofi-distribute-postprocessing postprocessing/main.cpp postprocessing/postprocessing.h postprocessing/postprocessing.cpp)
target_include_directories(rofi-distribute-postprocessing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../extlib/)
target_link_libraries(rofi-distribute-postprocessing reconfig)

add_executable(rofi-distribute-preprocessing preprocessing/main.cpp)
target_include_directories(rofi-distribute-preprocessing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../extlib/)
target_link_libraries(rofi-distribute-preprocessing reconfig)