cmake_minimum_required(VERSION 3.11)


set(WORLDS
    spider
    50-init
)


add_custom_target(worlds-all ALL)

foreach(WORLD IN LISTS WORLDS)
    create_world(${WORLD}.world ${WORLD}.in)
endforeach()
