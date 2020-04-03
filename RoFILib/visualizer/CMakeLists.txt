cmake_minimum_required(VERSION 3.6)
project(rofi)

add_resources(modelResources "model/body.obj" "model/connector.obj" "model/shoe.obj")

add_executable(rofi-vis ${modelResources} ${ARMADILLO_LIBRARIES}
    main.cpp Generator.h Camera.h Animator.h)
target_link_libraries(rofi-vis PUBLIC configuration atoms cxxopts ${VTK_LIBRARIES})

add_executable(rofi-vis-test ${modelResources} test/main.cpp test/test.cpp)
target_link_libraries(rofi-vis-test PUBLIC configuration atoms ${ARMADILLO_LIBRARIES} ${VTK_LIBRARIES} Catch2::Catch2)
target_include_directories(rofi-vis-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})



find_package(Qt5 COMPONENTS Widgets REQUIRED)

if(Qt5_FOUND)
  
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)

  add_executable(rofi-app
    rofiappmainwindow.cpp rofiapp.cpp ${modelResources} )
    
  target_link_libraries(rofi-app
    Qt5::Widgets configuration atoms cxxopts ${VTK_LIBRARIES})
  
endif()

