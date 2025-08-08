#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

mkdir -p ./obj
rm -rf ./obj/vmlinuz
rm -rf ./obj/cpio
mkdir -p ./obj/cpio
cd ./obj/cpio

mkdir -p -m 700 ./proc
mkdir -p -m 700 ./sys
mkdir -p -m 700 ./dev
mkdir -p -m 700 ./usr/bin
ln -s usr/bin ./bin
ln -s usr/bin ./sbin
ln -s bin ./usr/sbin
mkdir -p -m 700 ./lib
mkdir -p -m 700 ./lib64

mkdir -p -m 700 ./lib/modules
cp -r "/lib/modules/$(uname -r)/" "./lib/modules/$(uname -r)/"
mv "./lib/modules/$(uname -r)/vmlinuz" ../vmlinuz
find . -name "*.ko.zst" -exec zstd -d --rm  {} \;
depmod -a -v --moduledir="$(realpath ./lib/modules)"

cp "$(which busybox)" ./usr/bin/busybox
chmod 700 ./usr/bin/busybox
cp ../../../payload/build/bin/mystery ./usr/bin/mystery
chmod 700 ./usr/bin/mystery
cp ../../resources/init.sh ./init
chmod 700 ./init