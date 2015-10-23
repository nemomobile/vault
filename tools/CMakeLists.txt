configure_file(vault-gc.service.in vault-gc.service @ONLY)

install(
  PROGRAMS git-vault-gc gc-default git-vault-snapshot-units git-vault-export
  DESTINATION ${TOOLS_DIR}
  )

install(
  FILES git-vault-rebase-generate.awk git-vault-rebase-prepare.awk vault-misc
  DESTINATION ${TOOLS_DIR}
  )
