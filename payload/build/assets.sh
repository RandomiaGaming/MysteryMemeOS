#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

# Given an assetPath creates an asset c and asset h file.
precompile_asset() {
    assetPath="$1"
    assetName="$(basename "$assetPath")"
    assetName="${assetName//./_}"
    assetCPath="./assets_c/$assetName.c"
    assetHPath="./assets_h/$assetName.h"

    if [[ -f "$assetCPath" ]]; then
        assetTime="$(stat -c %Y "$assetPath")"
        assetCTime="$(stat -c %Y "$assetCPath")"
        if [[ "$assetCTime" -ge "$assetTime" ]]; then
            echo "Skipping \"$assetPath\" because it has not been modified."
            return
        fi
    fi

    echo "Precompiling asset \"$assetPath\"."

    length="$(stat --format=%s "$assetPath")"
    buffer="$(xxd -p -u -c1 "$assetPath" | awk 'NF { printf "0x%s, ", $0 }')"
    buffer="${buffer::-2}"

    mkdir -p ./assets_c
    mkdir -p ./assets_h

    echo "#ifndef $(echo "$assetName")_h" >$assetHPath
    echo "#define $(echo "$assetName")_h" >>$assetHPath
    echo "#include <stddef.h>" >>$assetHPath
    echo "extern const size_t $(echo "$assetName")_length;" >>$assetHPath
    echo "extern const unsigned char $(echo "$assetName")_buffer[];" >>$assetHPath
    echo "#endif" >>$assetHPath

    echo "#include <stddef.h>" >$assetCPath
    echo "const size_t $(echo "$assetName")_length = $(echo "$length");" >>$assetCPath
    echo "const unsigned char $(echo "$assetName")_buffer[] = { $(echo "$buffer") };" >>$assetCPath
}

precompile_asset ../assets/mysteryimage.bmp
precompile_asset ../assets/mysterysong.wav