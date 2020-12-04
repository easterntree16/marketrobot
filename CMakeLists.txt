
cmake_minimum_required(VERSION 3.14)

# Project
project(MarketRobot)

MESSAGE(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
MESSAGE(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")
set(CMAKE_CXX_STANDARD 17)

INCLUDE_DIRECTORIES(3rd_party/spdlog/include)
INCLUDE_DIRECTORIES(3rd_party/rapidjson/include)
INCLUDE_DIRECTORIES(3rd_party/yaml-cpp/include)
LINK_DIRECTORIES(./lib)
ADD_SUBDIRECTORY("src")
ADD_SUBDIRECTORY("test")


