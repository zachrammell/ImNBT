cmake_minimum_required(VERSION 3.2)

project(ImNBT)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(IMNBT_SOURCES
  "include/ImNBT/NBTReader.hpp"
  "include/ImNBT/NBTWriter.hpp"
  "include/ImNBT/NBTBuilder.hpp"
  "include/ImNBT/NBTRepresentation.hpp"
  "src/byteswapping.h"
  "src/NBTReader.cpp"
  "src/NBTWriter.cpp"
  "src/NBTBuilder.cpp"
  "src/NBTRepresentation.cpp"
  )

 set(IMNBT_NATVIS
  "natvis/imnbt.natvis"
 )

add_library(ImNBT ${IMNBT_SOURCES} ${IMNBT_NATVIS})
target_include_directories(ImNBT PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_options(ImNBT PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)
target_compile_definitions(ImNBT PRIVATE $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)

option(IMNBT_USE_ZLIB "Use zlib for compressed NBT" ON)
if (IMNBT_USE_ZLIB)
  set(ZLIB_COMPAT ON)
  set(ZLIB_ENABLE_TESTS OFF)
  set(IMNBT_OLD_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
  set(BUILD_SHARED_LIBS false)
  add_subdirectory("external/zlib")
  set(BUILD_SHARED_LIBS ${IMNBT_OLD_BUILD_SHARED_LIBS})
  target_link_libraries(ImNBT PRIVATE zlibstatic)
  target_include_directories(ImNBT PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/zlib")
endif()

# define the macro DEBUG if compiling in that configuration
target_compile_definitions(ImNBT PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")

# Build tests by default, make it a configurable option
option(IMNBT_BUILD_TESTS "Build ImNBT tests" ON)
if (IMNBT_BUILD_TESTS)
  add_subdirectory("test")
endif()
