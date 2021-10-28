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
  "include/ImNBT/NBTRepresentation.hpp"
  "src/NBTRepresentation.cpp"
 )

 set(IMNBT_NATVIS
  "natvis/imnbt.natvis"
 )

add_library(ImNBT ${IMNBT_SOURCES} ${IMNBT_NATVIS})
target_include_directories(ImNBT PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Build tests by default, make it a configurable option
option(IMNBT_BUILD_TESTS "Build ImNBT tests" ON)
if (IMNBT_BUILD_TESTS)
  add_subdirectory("test")
endif()
