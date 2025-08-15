#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

# Clones and builds libalsa.
# Doesn't care if libalsa was already built it always resets everything and starts fresh.
rm -rf ./libalsa
git clone https://github.com/alsa-project/alsa-lib.git ./libalsa
cd ./libalsa
autoreconf -i
CC="$(realpath "../musl/bin/gcc")" ./configure --prefix="" --disable-shared --enable-static --disable-python --disable-hwdep --disable-topology --disable-aload --disable-mixer --disable-seq --disable-rawmidi --disable-ucm --disable-old-symbols --without-versioned
make -j$(nproc)
make DESTDIR="$(realpath "./build")" install
rm -rf ./build/bin
rm -rf ./build/lib/pkgconfig
rm -f ./build/lib/libasound.la
rm -rf ./build/share/aclocal