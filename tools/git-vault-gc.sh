#!/bin/bash

function error {
    echo "`basename $0`: Error: ${@:1}" 1>&2;
    exit 1
}

ARGS=$(getopt -o fdD -- "$@");

if [ $? -ne 0 ]; then
  exit 1
fi

force=false
dump=false
dump_intention=false

while true; do
    case "$1" in
        -f)
            force=true
            shift;
            ;;
        -d)
            dump=true
            shift;
            ;;
        -D)
            dump_intention=true
            shift;
            ;;
        *)
            if [ $# -eq 0 ]; then
                break;
            fi
            error "Unknown option $1"
            break;
    esac
done

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

function gen_cmd {
    cat $cmd_file | gawk -f $src/git-vault-rebase-generate.awk
}

if ($dump || $dump_intention); then
    $dump_intention && cat $cmd_file
    $dump && gen_cmd
    exit 0
fi

if ! grep '^\(old_tag\|skip\)' $cmd_file; then
    echo "There is no need in gc for the $root"
    if ! $force; then
        exit 0
    fi
    echo "Forcing gc"
fi

cmd=$(gen_cmd)

function git_gc {
    echo "GC"
    git prune
    git gc --aggressive
    echo "PRUNE+"
    git prune
}

function rollback {
    echo "ROLLBACK"
    git reset --hard
    git clean -fd
    git checkout master
    git branch -D migrate
    for t in $(git tag | grep '^migrate/'); do
        git tag -d "$t"
    done
    git_gc
    error "during migration"
}

export -f rollback

eval "$cmd" || error "$cmd"

echo "REPLACE MASTER"
# do it separetely to be on the safe side
(git branch -D master && git branch -m migrate master) || error "replacing master"

echo "CLEAR REFLOG"
git reflog expire --expire=now --all || error "exiring reflog"

echo "REMOVING DANGLING BLOBS"
for obj in $(git fsck --unreachable master \
    | grep '^unreachable blob' \
    | sed 's/unreachable blob //'); do
    for blob in $(git show $obj \
        | head -n 1 \
        | grep '\.\./\.git/blobs' \
        | sed 's:^[./]*\.git/blobs:.git/blobs:'); do
        echo "Orphan $blob"
        (test -f $blob && (rm $blob -f || echo "Can't remove $blob")) \
            || echo "No such blob $blob"
    done
done
git_gc
echo "OK"
