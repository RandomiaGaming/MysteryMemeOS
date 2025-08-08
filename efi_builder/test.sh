#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

cp /usr/share/edk2-ovmf/x64/OVMF_VARS.4m.fd ./obj/OVMF_VARS.4m.fd
qemu-system-x86_64 \
  -enable-kvm \
  -cpu host \
  -smp cores=4 \
  -m 4G \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2-ovmf/x64/OVMF_CODE.4m.fd \
  -drive if=pflash,format=raw,file=./obj/OVMF_VARS.4m.fd \
  -kernel ./obj/mystery.efi \
  -display sdl -vga std \
  -audiodev pa,id=snd0,out.frequency=48000 -device ich9-intel-hda -device hda-output,audiodev=snd0 \
  -name "MysteryMemeOS"