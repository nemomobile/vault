#!/bin/bash

initial_dir=$(pwd)

src=$(dirname $0)
src=$(cd $src;pwd)
source $src/testing-common-vault.sh

OPTS=$(getopt -o A:D:B:H: --long action:,dir:,bin-dir:,home-dir: -n "$0" -- "$@")
[ $? -eq 0 ] || error 3 "Error parsing options $@"

eval set -- "$OPTS"

while true; do
    case "$1" in
        -A | --action)
            action=$2
            shift 2
            ;;
        -D | --dir)
            data_dir=$2
            shift 2
            ;;
        -B | --bin-dir)
            blobs_dir=$2
            shift 2
            ;;
        -H | --home-dir)
            home_dir=$2
            shift 2
            ;;
        -- )
            shift; break ;;
        *)
            shift; break ;;
    esac
done

trace "Options: action=$action, data=$data_dir, blobs=$blobs_dir, home=$home_dir"

[[ "$action" =~ ^import|export$ ]] || error 31 "Unknown action: $action"
for d in data_dir blobs_dir home_dir; do
    [ -d ${!d} ] || error 32 "Dir doesn't exist: $d=${!d}"
done

