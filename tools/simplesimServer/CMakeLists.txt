cmake_minimum_required(VERSION 3.11)


set (SRC
    main.cpp
)

add_executable(rofi-simplesimServer ${SRC})
target_link_libraries(rofi-simplesimServer PUBLIC simplesim simplesimServer messageServer configurationWithJson simplesimPyFilter dimcli)
