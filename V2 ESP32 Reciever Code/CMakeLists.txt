cmake_minimum_required(VERSION 3.0.0)
project(ESP 32 Reciever Code VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(ESP 32 Reciever Code ESP 32 Reciever Code.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
