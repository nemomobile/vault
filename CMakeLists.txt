PROJECT(VAULT)
cmake_minimum_required(VERSION 2.8.8)

IF(NOT DEFINED VERSION OR VERSION STREQUAL "")
message(FATAL_ERROR "Define VERSION")
ENDIF(NOT DEFINED VERSION OR VERSION STREQUAL "")

IF(NOT DEFINED LONG_VERSION OR LONG_VERSION STREQUAL "")
set(LONG_VERSION ${VERSION})
ENDIF(NOT DEFINED LONG_VERSION OR LONG_VERSION STREQUAL "")

option(ENABLE_MULTIARCH "Enable support for multi-arch distros (lib64)" OFF)
IF(ENABLE_MULTIARCH)
  IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(LIB_SUFFIX "")
  ELSE(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(LIB_SUFFIX 64)
  ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
ELSE()
  SET(LIB_SUFFIX "")
ENDIF()
set(DST_LIB lib${LIB_SUFFIX})

message(STATUS "Version ${VERSION}")
message(STATUS "Long version is ${LONG_VERSION}")
message(STATUS "Multiarch is ${ENABLE_MULTIARCH}")

find_package(PkgConfig REQUIRED)
find_package(Qt5Core REQUIRED)

set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -fPIC -W -Wall -Wextra -Werror -g -O2 -std=c++0x")

#-Wno-psabi is to remove next g++ warning/note:
#the mangling of 'va_list' has changed in GCC 4.4
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -Wno-psabi")

# fix for gcc 4.6 specific compiling/linking issues, no dwarf-4
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -gdwarf-3"
  )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(tests)
add_subdirectory(units)
