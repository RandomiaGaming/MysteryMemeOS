#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

rm -rf ./libalsa
rm -rf ./libalsa_build
git clone https://github.com/alsa-project/alsa-lib.git ./libalsa
cd ./libalsa
autoreconf -i
CC="$(realpath "../musl/bin/gcc")" ./configure --prefix="$(realpath "../libalsa_build")" --disable-shared --enable-static --disable-python --disable-hwdep --disable-topology --disable-aload --disable-mixer --disable-seq --disable-rawmidi --disable-ucm --disable-old-symbols --without-versioned
make -j$(nproc)
make install
cd ../
rm -rf ./libalsa_build/bin
rm -rf ./libalsa_build/lib/pkgconfig
rm -f ./libalsa_build/lib/libasound.la
rm -rf ./libalsa_build/share/aclocal