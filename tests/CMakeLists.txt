pkg_check_modules(TUT REQUIRED tut>=0.0.3)
include_directories(${TUT_INCLUDES})

find_package(Qt5Core REQUIRED)
set(CMAKE_AUTOMOC TRUE)

set(TESTS_DIR /opt/tests/vault)
set(UNIT_TESTS unit vault transfer)

add_executable(unit_all unit_all.cpp)
target_link_libraries(unit_all vault-unit)
qt5_use_modules(unit_all Core)
install(TARGETS unit_all DESTINATION ${TESTS_DIR})

MACRO(UNIT_TEST _name)
  set(_exe_name test_${_name})
  add_executable(${_exe_name} main.cpp ${_name}.cpp tests_common.cpp)
  target_link_libraries(${_exe_name}
    ${QTAROUND_LIBRARIES}
    ${COR_LIBRARIES}
    vault-core
    )
  qt5_use_modules(${_exe_name} Core)
  install(TARGETS ${_exe_name} DESTINATION ${TESTS_DIR})
ENDMACRO(UNIT_TEST)

foreach(t ${UNIT_TESTS})
  UNIT_TEST(${t})
endforeach(t)

target_link_libraries(test_transfer vault-transfer)

MACRO(UNIT_IMPL _name)
  set(_exe_name ${_name}_vault_test)
  set(UNIT_NAME ${_name})
  configure_file(basic_unit.cpp.in  ${_name}_unit.cpp @ONLY)
  add_executable(${_exe_name} ${_name}_unit.cpp)
  target_link_libraries(${_exe_name} vault-unit)
  qt5_use_modules(${_exe_name} Core)
  install(TARGETS ${_exe_name} DESTINATION ${TESTS_DIR})
ENDMACRO(UNIT_IMPL)

UNIT_IMPL(unit1)
UNIT_IMPL(unit2)

configure_file(tests.xml.in tests.xml @ONLY)
install(FILES tests.xml DESTINATION ${TESTS_DIR})
install(PROGRAMS check_dirs_similar.sh DESTINATION ${TESTS_DIR})

add_subdirectory(qml)
