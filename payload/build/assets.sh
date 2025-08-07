#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

# assetPath assetName outputCPath outputHPath
precompile() {
    assetPath="$1"
    assetName="$2"
    outputCPath="$3"
    outputHPath="$4"
    length="$(stat --format=%s "$assetPath")"
    buffer="$(xxd -p -u -c1 "$assetPath" | awk 'NF { printf "0x%s, ", $0 }')"
    buffer="${buffer::-2}"

    echo "#ifndef $(echo "$assetName")_H" >$outputHPath
    echo "#define $(echo "$assetName")_H" >>$outputHPath
    echo "#include <stddef.h>" >>$outputHPath
    echo "extern const size_t $(echo "$assetName")_length;" >>$outputHPath
    echo "extern const unsigned char $(echo "$assetName")_buffer[];" >>$outputHPath
    echo "#endif" >>$outputHPath

    echo "#include <stddef.h>" >$outputCPath
    echo "const size_t $(echo "$assetName")_length = $(echo "$length");" >>$outputCPath
    echo "const unsigned char $(echo "$assetName")_buffer[] = { $(echo "$buffer") };" >>$outputCPath
}

rm -rf ../assets/c
mkdir -p ../assets/c
rm -rf ../assets/h
mkdir -p ../assets/h

precompile ../assets/raw/mysteryimage.bmp mysteryimage ../assets/c/mysteryimage.c ../assets/h/mysteryimage.h
precompile ../assets/raw/mysterysong.wav mysterysong ../assets/c/mysterysong.c ../assets/h/mysterysong.h