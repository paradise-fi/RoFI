cmake_minimum_required(VERSION 3.6)
project(rofi)

set(CMAKE_CXX_STANDARD 17)

if(${LIBCXX})
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
endif(${LIBCXX})

add_executable(rofi-generate ${ARMADILLO_LIBRARIES} main.cpp)
target_link_libraries(rofi-generate configuration)
target_include_directories(rofi-generate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../extlib/)
