cmake_minimum_required(VERSION 3.2)

project(ImNBT)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(IMNBT_SOURCES
  "include/ImNBT/NBTReader.hpp"
  "include/ImNBT/NBTWriter.hpp"
  "src/byteswapping.h"
  "src/NBTReader.cpp"
  "src/NBTWriter.cpp"
)

add_library(ImNBT ${IMNBT_SOURCES})
target_include_directories(ImNBT PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
