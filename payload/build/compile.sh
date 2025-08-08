#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

# sourcePath
compile() {
    sourcePath="$1"
    sourceName="$(basename -s ".c" "$sourcePath")"
    sourceName="${sourceName//./_}"
    sourceOPath="./obj/$sourceName.o"

    if [[ -f "$sourceOPath" ]]; then
        sourceTime="$(stat -c %Y "$sourcePath")"
        sourceOTime="$(stat -c %Y "$sourceOPath")"
        if [[ "$sourceOTime" -ge "$sourceTime" ]]; then
            echo "Skipping \"$sourcePath\" because it has not been modified."
            return
        fi
    fi

    echo "Compiling source \"$sourcePath\"."

    mkdir -p ./obj

    ./musl/bin/gcc -c -DLINUX -g -O0 -no-pie -fno-pic -fno-plt -Wall -Wextra -Werror -std=c2x -I"./libdrm_build/include" -I"./libdrm_build/include/libdrm" -I"./libalsa_build/include" "$sourcePath" -o "$sourceOPath"
}

compile ./assets_c/mysteryimage_bmp.c
compile ./assets_c/mysterysong_wav.c
compile ../src/mystery.c