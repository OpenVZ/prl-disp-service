#!/bin/sh -x

export PATH=/usr/bin:/usr/sbin:/bin:/sbin

mount -t proc -o nosuid,noexec,nodev proc /proc
mount -t sysfs -o nosuid,noexec,nodev sysfs /sys
mount -t devtmpfs -o mode=0755,nosuid devtmpfs /dev

udevd --daemon --resolve-names=never
udevadm settle

python /tests/00main.py

# let test serial log drain
sleep 1
# power off the system
echo o > /proc/sysrq-trigger
# if init quits before poweroff the kernel would panic
sleep 10
