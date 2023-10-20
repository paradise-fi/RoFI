cmake_minimum_required(VERSION 3.6)
project(rofi)


file( GLOB ISOMORPHIC_SRC src/* )
add_library( isoreconfig STATIC ${ISOMORPHIC_SRC} )
target_include_directories( isoreconfig PUBLIC include )
target_link_libraries( isoreconfig PUBLIC ${ARMADILLO_LIBRARIES} configuration parsing nlohmann_json::nlohmann_json )

add_executable( rofi-isoreconfig main.cpp )
target_link_libraries( rofi-isoreconfig PRIVATE ${ARMADILLO_LIBRARIES} dimcli atoms-heavy configuration isoreconfig parsing )

add_executable( test-isoreconfig tests.cpp )
target_link_libraries( test-isoreconfig PRIVATE ${ARMADILLO_LIBRARIES} configuration isoreconfig configurationWithJson )
