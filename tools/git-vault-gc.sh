#!/bin/bash

function error {
    echo "`basename $0`: Error: ${@:1}" 1>&2;
    exit 1
}

src=$(dirname $0)
src=$(cd $src;pwd)

root=$(git rev-parse --show-toplevel)
rc=$?

if [ $rc -ne 0 ] || [ "x$root" == "x" ]; then
    error "Not a git $root"
fi

test -d "$root" || error "Not a dir $root"
test -e "$root/.vault" || error "Not a vault $root"

cd $root || error "Can't enter $root"

cmd_file=$(mktemp)
git log --pretty=format:%H$%d$%s \
    | gawk -f $src/git-vault-rebase-prepare.awk \
    | tac > $cmd_file
trap "rm $cmd_file" EXIT

if ! grep '^old_tag' $cmd_file; then
    echo "There is no need in gc for the $root"
    exit 1
fi

cmd=$(cat $cmd_file | gawk -f $src/git-vault-rebase-generate.awk)

eval "$cmd" || error "$cmd"
git reflog expire --expire=now --all || error "exiring reflog"

for obj in $(git fsck --unreachable master \
    | grep '^unreachable blob' \
    | sed 's/unreachable blob //'); do
    for blob in $(git show $obj \
        | head -n 1 \
        | grep '\.\./\.\./\.git/blobs' \
        | sed 's:^[./]*\.git/blobs:.git/blobs:'); do
        echo "Orphan $blob"
        rm $blob -f || echo "Can't remove $blob"
    done
done
git prune
git gc --aggressive

