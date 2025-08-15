#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

# Reset for new generation
mkdir -p ./obj
rm -rf ./obj/vmlinuz
rm -rf ./obj/cpio
mkdir -p ./obj/cpio
cd ./obj/cpio

# Create mount points
mkdir -p -m 700 ./proc
mkdir -p -m 700 ./sys
mkdir -p -m 700 ./dev

# Create basic file structure
mkdir -p -m 700 ./usr/bin
mkdir -p -m 700 ./usr/share
mkdir -p -m 700 ./usr/lib
mkdir -p -m 700 ./usr/lib/modules

# Make symlinks
ln -s usr/bin ./bin
ln -s usr/bin ./sbin
ln -s bin ./usr/sbin
ln -s usr/share ./share
ln -s usr/lib ./lib
ln -s usr/lib ./lib64
ln -s lib ./usr/lib64

# Copy and extract kernel modules and kernel
cp -r "/lib/modules/$(uname -r)/" "./lib/modules/$(uname -r)/"
mv "./lib/modules/$(uname -r)/vmlinuz" ../vmlinuz
find . -name "*.ko.zst" -exec zstd -d --rm  {} \;
depmod -a -v --moduledir="$(realpath ./lib/modules)"

# Copy libasound config files
cp -r ../../../payload/build/libalsa/build/share/* ./usr/share/

# Copy programs but save init and mystery for later
cp "$(which busybox)" ./usr/bin/busybox
chmod 700 ./usr/bin/busybox