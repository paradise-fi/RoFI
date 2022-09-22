file(GLOB_RECURSE SRC src/*.cpp)

add_library(esp32++ ${SRC})
set_property(TARGET esp32++ PROPERTY CXX_STANDARD 17)
target_include_directories(esp32++ PUBLIC include)
target_link_libraries(esp32++ PUBLIC idf::driver atoms)
target_compile_options(esp32++ PUBLIC -mtext-section-literals -mlongcalls)
