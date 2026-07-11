#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

# Reset for new generation
mkdir -p ./obj
rm -rf ./obj/vmlinuz
#rm -rf ./obj/cpio
#mkdir -p ./obj/cpio
cd ./obj/cpio

# Create mount points
#mkdir -p -m 700 ./proc
#mkdir -p -m 700 ./sys
#mkdir -p -m 700 ./dev

# Create basic file structure
#mkdir -p -m 700 ./usr/bin
#mkdir -p -m 700 ./usr/share
#mkdir -p -m 700 ./usr/lib

# Make symlinks
#ln -s usr/bin ./bin
#ln -s usr/bin ./sbin
#ln -s bin ./usr/sbin
#ln -s usr/share ./share
#ln -s usr/lib ./lib
#ln -s usr/lib ./lib64
#ln -s lib ./usr/lib64

# Copy and extract kernel modules and kernel
file vmlinuz | grep -oP 'version \K[^ ]+'
mkdir -p -m 700 ./usr/lib/modules
cp -r "/usr/lib/modules/$(uname -r)/" "./usr/lib/modules/$(uname -r)/"
mv "./usr/lib/modules/$(uname -r)/vmlinuz" ../vmlinuz
depmod -a --basedir="./" --moduledir="./usr/lib/modules"
cp -r ../../resources/firmware ./usr/lib/firmware

# Copy programs but save init and mystery for later
cp "$(which busybox)" ./usr/bin/busybox
chmod 700 ./usr/bin/busybox
while IFS= read -r line; do
    busybinPath="./usr/bin/$line"
    if [ ! -f "$busybinPath" ]; then
        ln -s busybox "$busybinPath"
    fi
done < <(busybox --list)

cp /usr/bin/kmod ./usr/bin/kmod
ln -s -f kmod ./usr/bin/depmod
ln -s -f kmod ./usr/bin/modprobe
ln -s -f kmod ./usr/bin/lsmod
ln -s -f kmod ./usr/bin/modinfo
cp /usr/lib/libzstd.so.1 ./usr/lib/libzstd.so.1
cp /usr/lib/liblzma.so.5 ./usr/lib/liblzma.so.5
cp /usr/lib/libz.so.1 ./usr/lib/libz.so.1
cp /usr/lib/libcrypto.so.3 ./usr/lib/libcrypto.so.3
cp /usr/lib/libgcc_s.so.1 ./usr/lib/libgcc_s.so.1
cp /usr/lib/libc.so.6 ./usr/lib/libc.so.6
cp /usr/lib/libbrotlienc.so.1 ./usr/lib/libbrotlienc.so.1
cp /usr/lib/libbrotlidec.so.1 ./usr/lib/libbrotlidec.so.1
cp /usr/lib64/ld-linux-x86-64.so.2 ./usr/lib64/ld-linux-x86-64.so.2
cp /usr/lib/libm.so.6 ./usr/lib/libm.so.6
cp /usr/lib/libbrotlicommon.so.1 ./usr/lib/libbrotlicommon.so.1

