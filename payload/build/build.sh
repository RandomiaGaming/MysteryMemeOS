#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

if [[ "$#" -gt "0" ]] && [[ "$1" == "debug" ]]; then
    debug="true"
else
    debug="false"
fi

./assets.sh
./compile.sh
./link.sh "$@"