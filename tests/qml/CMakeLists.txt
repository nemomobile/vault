add_executable(test_vault_qml main.cpp)
set_target_properties(test_vault_qml
  PROPERTIES COMPILE_DEFINITIONS
  "TEST_DIR=\"${TESTS_DIR}\"; IMPORT_DIR=\"${CMAKE_INSTALL_PREFIX}/lib/vault/qml\""
  )
qt5_use_modules(test_vault_qml Core)
install(TARGETS test_vault_qml DESTINATION ${TESTS_DIR})
install(FILES tst_vault.qml DESTINATION ${TESTS_DIR})
