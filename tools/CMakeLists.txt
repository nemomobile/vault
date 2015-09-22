configure_file(vault-gc.service.in vault-gc.service @ONLY)

install(
  PROGRAMS
  git-vault-gc gc-default
  git-vault-root
  git-vault-snapshot-units
  DESTINATION ${TOOLS_DIR}
  )

install(
  FILES
  git-vault-rebase-generate.awk git-vault-rebase-prepare.awk
  vault-misc
  vault-unit-common
  DESTINATION
  ${TOOLS_DIR}
  )
