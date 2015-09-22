#!/bin/bash

initial_dir=$(pwd)

src=$(dirname $0)
src=$(cd $src;pwd)
source $src/testing-common-vault.sh
unit_common_file=$lib_dir/vault-unit-common
[[ -e $unit_common_file ]] || error 33 "There is no $unit_common_file"
source $unit_common_file

