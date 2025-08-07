#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

debug="true"

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

    if "$debug" == "true"; then
        ./musl/bin/gcc -c -DLINUX -g -O0 -Wall -Wextra -Werror -std=c2x -I"./libdrm_build/include" -I"./libdrm_build/include/libdrm" -I"./libalsa_build/include" "$sourcePath" -o "$sourceOPath"
    else
        ./musl/bin/gcc -c -DLINUX -Wall -Wextra -Werror -std=c2x -I"./libdrm_build/include" -I"./libdrm_build/include/libdrm" -I"./libalsa_build/include" "$sourcePath" -o "$sourceOPath"
    fi
}

compile ../src/mystery.c

# make this work TODO
binaryPath="./bin/mystery"
objectPaths=$(printf '"%s" ' ./obj/*.o)
echo "Linking binary \"$binaryPath\"."
mkdir -p ./bin
if "$debug" == "true"; then
    ./musl/bin/gcc -static -static-libstdc++ -static-libgcc -g $objectPaths -o "$binaryPath"
else
    ./musl/bin/gcc -static -static-libstdc++ -static-libgcc $objectPaths -o "$binaryPath"
fi