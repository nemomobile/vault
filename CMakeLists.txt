PROJECT(VAULT)
cmake_minimum_required(VERSION 2.8.8)

message(STATUS "Tools go to ${TOOLS_DIR}")
set(prefix ${CMAKE_INSTALL_PREFIX})

IF(NOT DEFINED VERSION OR VERSION STREQUAL "")
message(FATAL_ERROR "Define VERSION")
ENDIF(NOT DEFINED VERSION OR VERSION STREQUAL "")

IF(NOT DEFINED LONG_VERSION OR LONG_VERSION STREQUAL "")
set(LONG_VERSION ${VERSION})
ENDIF(NOT DEFINED LONG_VERSION OR LONG_VERSION STREQUAL "")

message(STATUS "Version ${VERSION}")
message(STATUS "Long version is ${LONG_VERSION}")

find_package(PkgConfig REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Cor REQUIRED)

set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address -fno-omit-frame-pointer")

#-Wno-psabi is to remove next g++ warning/note:
#the mangling of 'va_list' has changed in GCC 4.4
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -Wno-psabi")

# fix for gcc 4.6 specific compiling/linking issues, no dwarf-4
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -gdwarf-3"
  )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

pkg_check_modules(QTAROUND qtaround REQUIRED)
include_directories(
  ${COR_INCLUDE_DIRS}
  ${QTAROUND_INCLUDE_DIRS}
)
link_directories(
  ${COR_LIBRARY_DIRS}
  ${QTAROUND_LIBRARY_DIRS}
)

add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(tests)
add_subdirectory(qml/Vault)
add_subdirectory(tools)

configure_install_pkgconfig(vault-unit)

install(
  DIRECTORY include/vault
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.hpp"
  PATTERN "*.h"
)
