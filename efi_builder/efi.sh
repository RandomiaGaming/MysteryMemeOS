#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

mkdir -p ./obj

cd ./obj/cpio
find . -print | cpio -ov --format=newc --owner=+0:+0 --file="../mystery.img"
cd ../../

gzip -f -k ./obj/mystery.img

echo "[UKI]" > ./obj/ukify.conf
echo "Linux=$(realpath ./obj/vmlinuz)" >> ./obj/ukify.conf
echo "Initrd=$(realpath ./obj/mystery.img.gz)" >> ./obj/ukify.conf
ukify -c ./obj/ukify.conf build -o ./obj/mystery.efi