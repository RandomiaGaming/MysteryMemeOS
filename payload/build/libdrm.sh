#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

rm -rf ./libdrm
rm -rf ./libdrm_build
git clone https://gitlab.freedesktop.org/mesa/libdrm.git ./libdrm
cd ./libdrm
CC="$(realpath "../musl/bin/gcc")" meson setup ./build --reconfigure --prefix="$(realpath "../libdrm_build")" --default-library=static -Dintel=disabled -Dradeon=disabled -Damdgpu=disabled -Dnouveau=disabled -Dvmwgfx=disabled -Detnaviv=disabled -Dfreedreno=disabled -Dexynos=disabled -Domap=disabled -Dtegra=disabled -Dcairo-tests=disabled -Dvalgrind=disabled -Dman-pages=disabled
ninja -j$(nproc) -C ./build
ninja -C ./build install
cd ../
rm -rf ./libdrm_build/lib/pkgconfig