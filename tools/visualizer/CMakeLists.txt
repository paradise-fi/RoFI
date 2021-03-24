cmake_minimum_required(VERSION 3.6)
project(rofi)

find_package(Armadillo REQUIRED)
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

add_resources(modelResources "model/body.obj" "model/connector.obj" "model/shoe.obj")

message(${modelResources})

FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG        v2.2.0
)
FetchContent_GetProperties(cxxopts)
if(NOT cxxopts_POPULATED)
  FetchContent_Populate(cxxopts)
  add_subdirectory(${cxxopts_SOURCE_DIR} ${cxxopts_BINARY_DIR})
endif()

add_executable(rofi-vis ${modelResources} ${ARMADILLO_LIBRARIES}
    main.cpp Generator.h Camera.h Animator.h)
target_link_libraries(rofi-vis PUBLIC configuration atoms atoms-heavy cxxopts ${VTK_LIBRARIES})

add_executable(test-rofi-vis ${modelResources} test/main.cpp test/test.cpp)
target_link_libraries(test-rofi-vis PUBLIC configuration atoms atoms-heavy ${ARMADILLO_LIBRARIES} ${VTK_LIBRARIES} Catch2::Catch2)
target_include_directories(test-rofi-vis PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rofi-vis-build-video
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/buildVideo.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rofi-vis-build-video
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/buildVideo.sh
	COMMENT "Building rofi-vis-build-video")

add_custom_target(rofi-vis-build-video
	DEPENDS rofi-vis ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rofi-vis-build-video)

add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rofi-vis-video
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/createAnimation.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rofi-vis-video
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/createAnimation.sh
	COMMENT "Building rofi-vis-video")

add_custom_target(rofi-vis-video
	DEPENDS rofi-vis ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rofi-vis-video ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rofi-vis-build-video)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

if(Qt5_FOUND)

  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)

  add_executable(rofi-app
    rofiapp_mainWindow.cpp rofiapp.cpp rofiapp_resources.qrc Vtk.cpp ${modelResources} )

  target_link_libraries(rofi-app
    Qt5::Widgets configuration atoms atoms-heavy cxxopts ${VTK_LIBRARIES})

endif()

