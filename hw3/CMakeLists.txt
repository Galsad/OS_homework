cmake_minimum_required(VERSION 3.8)
project(hw3)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp message_slot.c message_sender.c message_reader.c message_slot.h)
add_executable(hw3 ${SOURCE_FILES})