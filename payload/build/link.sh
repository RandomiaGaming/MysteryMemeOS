#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

if [[ "$#" -gt "0" ]] && [[ "$1" == "debug" ]]; then
    debug="true"
else
    debug="false"
fi

binaryPath="./bin/mystery"
mkdir -p ./bin
if "$debug" == "true"; then
    echo "Linking binary \"$binaryPath\" in Debug mode."
    ./musl/bin/gcc -static -static-libstdc++ -static-libgcc -g -no-pie -fno-pic -fno-plt ./obj/* -o "$binaryPath"
else
    echo "Linking binary \"$binaryPath\" in Release mode."
    ./musl/bin/gcc -static -static-libstdc++ -static-libgcc -s -no-pie -fno-pic -fno-plt ./obj/* -o "$binaryPath"
    objcopy \
      --only-section=.init \
      --only-section=.text \
      --only-section=.fini \
      --only-section=.rodata \
      --only-section=.init_array \
      --only-section=.fini_array \
      --only-section=.data \
      --only-section=.bss \
      --only-section=.data.rel.ro \
      --only-section=.eh_frame  \
      --only-section=.shstrtab  \
      ./bin/mystery ./bin/mystery
fi