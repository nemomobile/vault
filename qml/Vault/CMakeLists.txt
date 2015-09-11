
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC TRUE)

find_package(Qt5Qml REQUIRED)

add_library(vault-declarative SHARED plugin.cpp vault.cpp)
qt5_use_modules(vault-declarative Qml)
target_link_libraries(vault-declarative vault-core vault-transfer)

set_target_properties(vault-declarative PROPERTIES
  SOVERSION 0
  VERSION ${VERSION}
  )
install(TARGETS vault-declarative DESTINATION ${DST_LIB}/qt5/qml/NemoMobile/Vault)
install(FILES qmldir DESTINATION ${DST_LIB}/qt5/qml/NemoMobile/Vault)
