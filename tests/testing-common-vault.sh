#!/bin/bash

initial_dir=$(pwd)

src=$(dirname $0)
src=$(cd $src;pwd)
lib_dir=$src
if [ "x$VAULT_LIB_DIR" != "x" ]; then
    if ! [ -d $VAULT_LIB_DIR ]; then
        echo "No such dir $VAULT_LIB_DIR"
        exit 1
    fi
    lib_dir="$VAULT_LIB_DIR"
elif [ -d "$src/../tools" ]; then
    export VAULT_LIB_DIR=$src/../tools
    lib_dir="$VAULT_LIB_DIR"
fi
source $lib_dir/vault-misc

cur_test="Begin"
function NEXT_TEST {
    trace $@
    cur_test=$1
}

function test_failed {
    echo 'stdout {' >&2;
    cat $test_out >&2;
    echo '} stderr {' >&2;
    cat $test_err >&2;
    echo '}' >&2;
    error $1 "Test: $cur_test." ${@:2}
}

function check_is_vault_storage {
    d=$1
    [ -d $d ] || test_failed 7 "Not a dir $d"
    [ -f $d/config ] || test_failed 7 "Not a git storage"
    [ -d $d/blobs ] || test_failed 7 "Not a vault storage $d?"
}


function create_test_dir {
    test_dir=$(mktemp -t -d vault.XXXXXXXXXX)
    test_out=$test_dir/stdout.txt
    test_err=$test_dir/stderr.txt
    touch $test_out
    touch $test_err
    cd $test_dir || test_failed 1 "Can't enter $test_dir"
    normal_cleanup="$normal_cleanup; echo 'OK' 1>&2; cd $initial_dir; rm -rf $test_dir"
    failed_cleanup="warn 'Dir $test_dir is left';$failed_cleanup;"
}

test_home=
storage=

function create_test_dirs {
    ensure_param_count_exit_usage $# 1 "create_test_dirs prefix"
    prefix=$1
    test_home=$test_dir/$prefix/home
    storage=$test_dir/$prefix/vault
    mkdir -p $test_home || test_failed 1 "Can't create $test_home"
    mkdir -p $storage || test_failed 1 "Can't create $storage"
}
