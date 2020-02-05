# This file is not a part of the original distribution of the printf library
# (https://github.com/mpaland/printf), it was added externally to ease the
# integration into the project

cmake_minimum_required(VERSION 3.13)

project(printf)

add_library(printf printf.c)
target_include_directories(printf PUBLIC .)