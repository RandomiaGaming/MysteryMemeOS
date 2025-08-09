#!/bin/busybox ash
echo "Reached Target: Kernel Init"

# Setup basic environment
export PATH="/usr/bin"

# Mount essentials
busybox mount -t proc proc /proc -o nosuid,noexec,nodev
busybox mount -t sysfs sys /sys -o nosuid,noexec,nodev
busybox mount -t devtmpfs dev /dev -o nosuid,mode=0700

# Disable ctrl alt del early
echo "0" > /proc/sys/kernel/ctrl-alt-del

# Load essential modules for input
busybox modprobe serio_raw

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

echo "Reached Target: Init Complete"

# Launch shell for user
mystery

# Gracefully exit without kernel panic
echo 1 > /proc/sys/kernel/sysrq 
echo s > /proc/sysrq-trigger
echo u > /proc/sysrq-trigger
echo o > /proc/sysrq-trigger
while true; do
    sleep 60
done
