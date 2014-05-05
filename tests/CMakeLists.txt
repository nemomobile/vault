pkg_check_modules(TUT tut REQUIRED)
include_directories(${TUT_INCLUDES})

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
enable_testing()

set(UNIT_TESTS unit)

MACRO(UNIT_TEST _name)
  set(_exe_name test_${_name})
  add_executable(${_exe_name} EXCLUDE_FROM_ALL main.cpp ${_name}.cpp)
  target_link_libraries(${_exe_name} cor ${CMAKE_THREAD_LIBS_INIT})
  add_test(${_exe_name} ${_exe_name})
  add_dependencies(check ${_exe_name})
ENDMACRO(UNIT_TEST)

foreach(t ${UNIT_TESTS})
  UNIT_TEST(${t})
endforeach(t)
