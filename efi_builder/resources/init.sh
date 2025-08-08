#!/bin/busybox ash
set -x

echo "Reached Target: Kernel Init"

# Setup basic environment
export PATH="/usr/bin"

# Mount essentials
busybox mount -t proc proc /proc -o nosuid,noexec,nodev
busybox mount -t sysfs sys /sys -o nosuid,noexec,nodev
busybox mount -t devtmpfs dev /dev -o nosuid,mode=0700

# Bind self to stdin stdout and stderr
ln -sfT /proc/self/fd /dev/fd
ln -sfT /proc/self/fd/0 /dev/stdin
ln -sfT /proc/self/fd/1 /dev/stdout
ln -sfT /proc/self/fd/2 /dev/stderr

# Load essential modules for input
busybox modprobe usbhid
busybox modprobe hid_generic
busybox modprobe serio
busybox modprobe atkbd
busybox modprobe serio_raw
busybox modprobe i8042

# Load essential modules for sound
busybox modprobe snd
busybox modprobe snd_pcm
busybox modprobe snd_timer
busybox modprobe snd_hwdep
busybox modprobe snd_hda_codec
busybox modprobe snd_hda_codec_realtek
busybox modprobe snd_ac97_codec
busybox modprobe snd_hda_intel
busybox modprobe snd_usb_audio
busybox modprobe snd_ens1371

mdev -s

echo "Reached Target: Init Complete"

# Launch shell for user
ash 0</dev/stdin 1>/dev/stdout 2>/dev/stderr

# Gracefully exit without kernel panic
echo 1 > /proc/sys/kernel/sysrq 
echo s > /proc/sysrq-trigger
echo u > /proc/sysrq-trigger
echo o > /proc/sysrq-trigger
while true; do
    sleep 60
done
