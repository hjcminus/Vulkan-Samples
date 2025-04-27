cmake_minimum_required(VERSION 3.14)

# https://cmake.org/cmake/help/latest/manual/cmake-variables.7.html
#   PROJECT_SOURCE_DIR
#     This is the source directory of the last call to the project() 
#     command made in the current directory scope or one of its parents. 
#     Note, it is not affected by calls to project() made within a child 
#     directory scope (i.e. from within a call to add_subdirectory() from 
#     the current scope). 
#   PROJECT_BINARY_DIR  // point to the build folder
#     Full path to build directory for project.
#     This is the binary directory of the most recent project() command.


project (vk_samples 
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# enable FOLDER property
#   seperate projects into different groups
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

if (CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug")
endif ()

message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
	add_definitions(-D_DEBUG)
    message("Set cmake debug")
endif ()


# set output binary file directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# ========== zlib ==========

option(ZLIB_BUILD_EXAMPLES "Enable Zlib Examples" ON)
option(ZLIB_SHARED "Build zlib as a shared library" ON)

set(ZLIB_BUILD_EXAMPLES OFF)	# do not build example
set(ZLIB_SHARED OFF)	# do not compile as shared library

# set cmake global variable BUILD_SHARED_LIBS to OFF
#   do not build zlib as shared library
set(BUILD_SHARED_LIBS OFF)

# set cmake global variable BUILD_SHARED_LIBS
#   do install zlib library
set(SKIP_INSTALL_LIBRARIES ON)

add_subdirectory(third_party/zlib-1.3.1)

# ========== libpng ==========

# add options defined in lpng1647/CMakeLists.txt
option(PNG_TESTS "Build the libpng tests" ON)
option(PNG_TOOLS "Build the libpng tools" ON)

option(PNG_SHARED "Build libpng as a shared library" ON)
option(PNG_STATIC "Build libpng as a static library" ON)

set(PNG_TESTS OFF)	# do not compile png test
set(PNG_TOOLS OFF)	# do not compile png tools
set(PNG_SHARED OFF)

set(SKIP_INSTALL_ALL ON)

add_subdirectory(third_party/lpng1647)

# ========== samplers ==========

add_subdirectory(source/common)
add_subdirectory(source/triangle)
add_subdirectory(source/mesh_shader)
add_subdirectory(source/normal_mapping)
add_subdirectory(source/instancing)
add_subdirectory(source/shadow_map)
add_subdirectory(source/cascaded_shadow_maps)
add_subdirectory(source/cubemaps)