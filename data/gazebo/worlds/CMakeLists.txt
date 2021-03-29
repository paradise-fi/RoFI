cmake_minimum_required(VERSION 3.11)


set(WORLDS
    spider
    wheel
    50-init
)


foreach(WORLD IN LISTS WORLDS)
    add_gazebo_world(${WORLD}.world ${WORLD}.in)
endforeach()
