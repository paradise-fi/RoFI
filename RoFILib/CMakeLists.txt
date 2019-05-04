cmake_minimum_required(VERSION 3.10)
project(rofi)

set(CMAKE_CXX_STANDARD 17)

if(${LIBCXX})
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
endif(${LIBCXX})

find_package(Armadillo REQUIRED)
include_directories(${ARMADILLO_INCLUDE_DIRS})

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

add_executable(rofi-reconfig ${ARMADILLO_LIBRARIES} reconfig/main.cpp Matrix.h Configuration.h Reader.h reconfig/Algorithms.h visualizer/Visualizer.h Printer.h)

add_executable(rofi-generate ${ARMADILLO_LIBRARIES} generate/main.cpp)

add_executable(rofi-test ${ARMADILLO_LIBRARIES} test/main.cpp test/test.cpp)

add_executable(rofi-vis ${ARMADILLO_LIBRARIES} visualizer/main.cpp visualizer/Generator.h visualizer/Camera.h visualizer/Animator.h)

add_executable(rofi-distribute-postprocessing distribute/postprocessing/main.cpp distribute/postprocessing/postprocessing.h distribute/postprocessing/postprocessing.cpp)

target_link_libraries(rofi-reconfig ${ARMADILLO_LIBRARIES})
target_link_libraries(rofi-generate ${ARMADILLO_LIBRARIES})
target_link_libraries(rofi-test ${ARMADILLO_LIBRARIES} ${VTK_LIBRARIES})
target_link_libraries(rofi-vis ${ARMADILLO_LIBRARIES} ${VTK_LIBRARIES})

add_subdirectory(distribute)