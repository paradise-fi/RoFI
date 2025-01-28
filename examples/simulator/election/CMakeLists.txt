cmake_minimum_required(VERSION 3.11)

add_executable(electionConcept main.cpp)
target_link_libraries(electionConcept rofi::hal networking leadership)