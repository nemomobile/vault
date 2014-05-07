pkg_check_modules(COR cor REQUIRED)
pkg_check_modules(libgit libgit REQUIRED)

include_directories(
  ${COR_INCLUDE_DIRS} ${libgit_INCLUDE_DIRS}
)
link_directories(
  ${COR_LIBRARY_DIRS} ${libgit_LIBRARY_DIRS}
)

set(CMAKE_AUTOMOC TRUE)

add_library(cutespp SHARED debug.cpp os.cpp json.cpp sys.cpp subprocess.cpp)
qt5_use_modules(cutespp Core)
target_link_libraries(cutespp ${COR_LIBRARIES})
set_target_properties(cutespp PROPERTIES
  SOVERSION 0
  VERSION ${VERSION}
  )
install(TARGETS cutespp DESTINATION ${DST_LIB})

add_library(vault SHARED vault.cpp vault_config.cpp)
qt5_use_modules(vault Core)
target_link_libraries(vault cutespp ${libgit_LIBRARIES})
set_target_properties(vault PROPERTIES
  SOVERSION 0
  VERSION ${VERSION}
  )
install(TARGETS vault DESTINATION ${DST_LIB})

add_executable(vault-cli vault-cli.cpp)
target_link_libraries(vault-cli vault)
install(TARGETS vault-cli DESTINATION bin RENAME vault)

add_library(vault-unit SHARED unit.cpp)
qt5_use_modules(vault-unit Core)
target_link_libraries(vault-unit ${COR_LIBRARIES} cutespp)
set_target_properties(vault-unit PROPERTIES
  SOVERSION 0
  VERSION ${VERSION}
  )
install(TARGETS vault-unit DESTINATION ${DST_LIB})
