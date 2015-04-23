pkg_check_modules(GITTIN gittin REQUIRED)

include_directories(
  ${GITTIN_INCLUDE_DIRS}
)
link_directories(
  ${GITTIN_LIBRARY_DIRS}
)

set(CMAKE_AUTOMOC TRUE)

configure_file(config.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/config.hpp @ONLY)

add_library(vault-core SHARED vault.cpp vault_config.cpp)
qt5_use_modules(vault-core Core)
target_link_libraries(vault-core
  ${QTAROUND_LIBRARIES}
  ${GITTIN_LIBRARIES}
)
set_target_properties(vault-core PROPERTIES
  SOVERSION 0
  VERSION ${VERSION}
  )
install(TARGETS vault-core DESTINATION ${DST_LIB})

add_executable(vault-cli vault-cli.cpp)
target_link_libraries(vault-cli vault-core)
set_target_properties(vault-cli PROPERTIES OUTPUT_NAME vault)
install(TARGETS vault-cli DESTINATION bin)

add_library(vault-unit SHARED unit.cpp)
qt5_use_modules(vault-unit Core)
target_link_libraries(vault-unit
  ${COR_LIBRARIES}
  ${QTAROUND_LIBRARIES}
)
set_target_properties(vault-unit PROPERTIES
  SOVERSION 0
  VERSION ${VERSION}
  )
install(TARGETS vault-unit DESTINATION ${DST_LIB})

add_library(vault-transfer SHARED transfer.cpp)
qt5_use_modules(vault-transfer Core)
target_link_libraries(vault-transfer
  ${COR_LIBRARIES}
  ${QTAROUND_LIBRARIES}
  vault-core
)
set_target_properties(vault-transfer PROPERTIES
  SOVERSION 0
  VERSION ${VERSION}
  )
install(TARGETS vault-transfer DESTINATION ${DST_LIB})
