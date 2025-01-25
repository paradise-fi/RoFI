cmake_minimum_required(VERSION 3.11)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(FILES
        main/rofirun.h
        explorer/explorer.h
        moduleConsole/moduleconsole.h
        selectProgram/selectprogram.h

        main.cpp
        main/rofirun.cpp
        explorer/explorer.cpp
        moduleConsole/moduleconsole.cpp
        selectProgram/selectprogram.cpp

        main/rofirun.ui
        explorer/explorer.ui
        moduleConsole/moduleconsole.ui
        selectProgram/selectProgram.ui
)

add_executable(rofi-run main.cpp ${FILES})
target_link_libraries(rofi-run PUBLIC Qt5::Core Qt5::Widgets)
