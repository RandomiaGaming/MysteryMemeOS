#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

# sourcePath objectPath debug
compile() {
    sourcePath="$1"
    objectPath="$2"
    debug="$3"

    if "$debug" == "true"; then
        ./musl/bin/gcc -c -DLINUX -g -O0 -Wall -Wextra -Werror -std=c2x -I"./libdrm_build/include" -I"./libalsa_build/include" "$sourcePath" -o "$objectPath"
    else
        ./musl/bin/gcc -c -DLINUX -Wall -Wextra -Werror -std=c2x -I"./libdrm_build/include" -I"./libalsa_build/include" "$sourcePath" -o "$objectPath"
    fi
}

debug=true

rm -rf ../obj
mkdir -p ../obj
rm -rf ../bin
mkdir -p ../bin

compile ../assets/c/mysteryimage.c ../obj/mysteryimage.o "$debug"
compile ../assets/c/mysterysong.c ../obj/mysterysong.o "$debug"
compile ../src/mystery.c ../obj/mystery.o "$debug"
compile ../src/mysteryaudio.c ../obj/mysteryaudio.o "$debug"
compile ../src/mysteryvideo.c ../obj/mysteryvideo.o "$debug"

if "$debug" == "true"; then
    ./musl/bin/gcc -static -static-libstdc++ -static-libgcc -g "../obj/mysteryimage.o" "../obj/mysterysong.o" "../obj/mystery.o" "../obj/mysteryaudio.o" "../obj/mysteryvideo.o" -o "../bin/mystery"
else
    ./musl/bin/gcc -static -static-libstdc++ -static-libgcc "../obj/mysteryimage.o" "../obj/mysterysong.o" "../obj/mystery.o" "../obj/mysteryaudio.o" "../obj/mysteryvideo.o" -o "../bin/mystery"
fi