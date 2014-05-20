pkg_check_modules(COR cor REQUIRED)
pkg_check_modules(GITTIN gittin REQUIRED)

include_directories(
  ${COR_INCLUDE_DIRS} ${GITTIN_INCLUDE_DIRS}
)
link_directories(
  ${COR_LIBRARY_DIRS} ${GITTIN_LIBRARY_DIRS}
)

set(CMAKE_AUTOMOC TRUE)

add_library(qtaround SHARED debug.cpp os.cpp json.cpp sys.cpp subprocess.cpp util.cpp)
qt5_use_modules(qtaround Core)
target_link_libraries(qtaround ${COR_LIBRARIES})
set_target_properties(qtaround PROPERTIES
  SOVERSION 0
  VERSION ${VERSION}
  )
install(TARGETS qtaround DESTINATION ${DST_LIB})

add_library(vault-core SHARED vault.cpp vault_config.cpp)
qt5_use_modules(vault-core Core)
target_link_libraries(vault-core qtaround ${GITTIN_LIBRARIES})
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
target_link_libraries(vault-unit ${COR_LIBRARIES} qtaround)
set_target_properties(vault-unit PROPERTIES
  SOVERSION 0
  VERSION ${VERSION}
  )
install(TARGETS vault-unit DESTINATION ${DST_LIB})

add_library(vault-transfer SHARED transfer.cpp)
qt5_use_modules(vault-transfer Core)
target_link_libraries(vault-transfer ${COR_LIBRARIES} qtaround vault-core)
set_target_properties(vault-transfer PROPERTIES
  SOVERSION 0
  VERSION ${VERSION}
  )
install(TARGETS vault-transfer DESTINATION ${DST_LIB})
