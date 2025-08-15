#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

# Clones and builds libdrm.
# Doesn't care if libdrm was already built it always resets everything and starts fresh.
rm -rf ./libdrm
git clone https://gitlab.freedesktop.org/mesa/libdrm.git ./libdrm
cd ./libdrm
CC="$(realpath "../musl/bin/gcc")" meson setup ./ninja --reconfigure --prefix="/" --default-library=static -Dintel=disabled -Dradeon=disabled -Damdgpu=disabled -Dnouveau=disabled -Dvmwgfx=disabled -Detnaviv=disabled -Dfreedreno=disabled -Dexynos=disabled -Domap=disabled -Dtegra=disabled -Dcairo-tests=disabled -Dvalgrind=disabled -Dman-pages=disabled
ninja -j$(nproc) -C ./ninja
DESTDIR="$(realpath "./build")" ninja -C ./ninja install
rm -rf ./build/lib/pkgconfig