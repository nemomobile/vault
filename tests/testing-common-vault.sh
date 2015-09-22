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
test_set_num=0
function NEXT_TEST {
    trace "TEST: $@"
    test_set_num=$(expr $test_set_num + 1)
    cur_test=$1
}

cur_test_step=
function test_step {
    cur_test_step=$1
    trace "STEP: $cur_test/$1"
}

function test_failed {
    echo 'stdout {' >&2;
    cat $test_out >&2;
    echo '} stderr {' >&2;
    cat $test_err >&2;
    echo '}' >&2;
    error $1 "Test: $cur_test($cur_test_step)" ${@:2}
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


create_test_dirs_next() {
    create_test_dirs $test_set_num
}

user_name=User1
email=mail@user

function vault_create() {
    vault -V $storage -H $test_home -a init -g user.name=$user_name,user.email=$email \
        || test_failed 1 "Vault init is failed"
}

function cd_vault_and_check_it {
    cd $storage || failed 2 "No vault dir $storage"
    init_root_and_enter
    check_is_vault_storage $storage/.git
    cd .git || failed 4 "No .git"
    [ "$(git config user.name)" == "$user_name" ] || test_failed 5 "User name is wrong"
    [ "$(git config user.email)" == "$email" ] || test_failed 5 "User email is wrong"
    cd .. || test_failed 6 "Can't cd .."
}
