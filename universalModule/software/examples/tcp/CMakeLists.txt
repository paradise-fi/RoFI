cmake_minimum_required(VERSION 3.13)

set(EXTRA_COMPONENT_DIRS "../../RoFIDriver")

set(COMPONENTS "")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(tcpExample)
