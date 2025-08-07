#!/bin/bash

set -xeuo pipefail

cp ../payload/mystery initramfs/bin/mystery
cp init initramfs/init

cd initramfs
find . -print0 | cpio --null -ov --format=newc --file="../initramfs.img"
cd ../

#gzip initramfs.img

echo "[UKI]" > ukify.conf
echo "Linux=$(ls /usr/lib/modules/*/vmlinuz)" >> ukify.conf
echo "Initrd=$(realpath initramfs.img)" >> ukify.conf

ukify -c ukify.conf build -o mystery.efi
