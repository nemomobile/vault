
add_executable(tst_vault main.cpp)
set_target_properties(tst_vault PROPERTIES COMPILE_DEFINITIONS "TEST_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"; IMPORT_DIR=\"${CMAKE_INSTALL_PREFIX}/lib/vault/qml\"")
qt5_use_modules(tst_vault Core)
add_dependencies(check tst_vault)
add_test(tst_vault tst_vault)
