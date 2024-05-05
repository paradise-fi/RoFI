cmake_minimum_required(VERSION 3.11)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(FILES
        main.cpp
        rofirun.cpp
        rofirun.h
        rofirun.ui
)

add_executable(rofiRun main.cpp ${FILES})
target_link_libraries(rofiRun PUBLIC Qt5::Core Qt5::Widgets)
