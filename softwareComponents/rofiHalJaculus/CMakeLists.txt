add_library(rofiHalJaculus INTERFACE ${SRC})
add_library(rofi::hal::js ALIAS rofiHalJaculus)
target_include_directories(rofiHalJaculus INTERFACE .)
set_target_properties(rofiHalJaculus PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(rofiHalJaculus INTERFACE jac-machine jac-link quickjs)

target_compile_definitions(quickjs PUBLIC -DESP_PLATFORM) # ESP_PLATFORM