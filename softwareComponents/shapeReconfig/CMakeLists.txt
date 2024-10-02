cmake_minimum_required(VERSION 3.6)
project(rofi)


file( GLOB SHAPERECONFIG_SRC src/* )
add_library( shapereconfig STATIC ${SHAPERECONFIG_SRC} )
target_include_directories( shapereconfig PUBLIC include )
target_link_libraries( shapereconfig PUBLIC ${ARMADILLO_LIBRARIES} configuration parsing nlohmann_json::nlohmann_json )

add_executable( rofi-shapereconfig main.cpp )
target_link_libraries( rofi-shapereconfig PRIVATE ${ARMADILLO_LIBRARIES} dimcli atoms-heavy configuration shapereconfig parsing )

add_executable( test-shapereconfig tests.cpp )
target_link_libraries( test-shapereconfig PRIVATE ${ARMADILLO_LIBRARIES} configuration shapereconfig configurationWithJson )
