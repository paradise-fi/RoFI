cmake_minimum_required(VERSION 3.6)
project(rofi)

add_executable(rofi-freconfig reconfiguration.cpp)
target_include_directories(rofi-freconfig INTERFACE .)
target_link_libraries(rofi-freconfig PRIVATE kinematics legacy-configuration dimcli geometry)

add_executable(getlocal getlocal.cpp)
target_link_libraries(getlocal rofi::hal)

add_executable(rofi-manipulator CoFI.cpp)
target_link_libraries(rofi-manipulator kinematics configuration atoms dimcli
                                rofi::hal geometry parsing-lite planning)

add_executable(rofi-treeconfig treeReconfig.cpp)
target_link_libraries(rofi-treeconfig PUBLIC tree-reconfig atoms dimcli
                                rofi::hal geometry parsing-lite)