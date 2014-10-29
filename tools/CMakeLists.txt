configure_file(vault-gc.service.in vault-gc.service @ONLY)

install(
  PROGRAMS git-vault-gc gc-default
  DESTINATION libexec/vault
  )

install(
  FILES git-vault-rebase-generate.awk git-vault-rebase-prepare.awk
  DESTINATION libexec/vault
  )
