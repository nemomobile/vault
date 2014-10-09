install(
  PROGRAMS git-vault-gc
  DESTINATION libexec/vault
  )

install(
  FILES git-vault-rebase-generate.awk git-vault-rebase-prepare.awk
  DESTINATION libexec/vault
  )
