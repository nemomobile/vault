pkg_check_modules(TUT REQUIRED tut>=0.0.3)
include_directories(${TUT_INCLUDES})

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
enable_testing()

find_package(Qt5Core REQUIRED)
set(CMAKE_AUTOMOC TRUE)

set(UNIT_TESTS unit vault transfer cutes)

add_executable(unit_all unit_all.cpp)
target_link_libraries(unit_all vault-unit)
qt5_use_modules(unit_all Core)

MACRO(UNIT_TEST _name)
  set(_exe_name test_${_name})
  add_executable(${_exe_name} EXCLUDE_FROM_ALL main.cpp ${_name}.cpp)
  target_link_libraries(${_exe_name} qtaround cor vault-core)
  qt5_use_modules(${_exe_name} Core)
  add_test(${_exe_name} ${_exe_name})
  add_dependencies(check ${_exe_name})
ENDMACRO(UNIT_TEST)

foreach(t ${UNIT_TESTS})
  UNIT_TEST(${t})
endforeach(t)

add_executable(unit1_vault_test unit1_vault_test.cpp)
target_link_libraries(unit1_vault_test vault-unit)
qt5_use_modules(unit1_vault_test Core)
add_dependencies(check unit1_vault_test)

target_link_libraries(test_transfer vault-transfer)

add_executable(unit1 unit1.cpp)
target_link_libraries(unit1 vault-unit)
qt5_use_modules(unit1 Core)

add_subdirectory(qml)
