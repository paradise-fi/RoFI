file(GLOB_RECURSE SRC src/*.cpp src/*.c)

# Specify optional dependencies from ESP-IDF
set(COMPONENTS "")

idf_component_register(SRCS ${SRC}
    INCLUDE_DIRS "." "src"
    REQUIRES "driver")
set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PUBLIC -mtext-section-literals -mlongcalls)

add_subdirectory($ENV{ROFI_ROOT}/softwareComponents/atoms ${CMAKE_CURRENT_BINARY_DIR}/atoms)
add_subdirectory($ENV{ROFI_ROOT}/softwareComponents/lwip++ ${CMAKE_CURRENT_BINARY_DIR}/lwip++)

target_link_libraries(${COMPONENT_LIB} PUBLIC atoms lwip++)
