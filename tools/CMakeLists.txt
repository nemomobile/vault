configure_file(vault-gc.service.in vault-gc.service @ONLY)

install(
  PROGRAMS git-vault-gc gc-default
  DESTINATION ${TOOLS_DIR}
  )

install(
  FILES git-vault-rebase-generate.awk git-vault-rebase-prepare.awk
  DESTINATION ${TOOLS_DIR}
  )
