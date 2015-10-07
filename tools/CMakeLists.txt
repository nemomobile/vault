configure_file(vault-gc.service.in vault-gc.service @ONLY)

install(
  PROGRAMS
  git-vault-gc gc-default
  git-vault-root
  git-vault-snapshot-units
  DESTINATION ${TOOLS_DIR}
  )

install(
  PROGRAMS
  git-remote-vault
  DESTINATION bin
  )

install(
  FILES
  git-vault-rebase-generate.awk
  git-vault-rebase-prepare.awk
  git-vault-split-refs.awk
  vault-misc
  vault-unit-common
  git-vault-helpers-remote
  DESTINATION
  ${TOOLS_DIR}
  )
