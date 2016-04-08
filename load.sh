#!/bin/sh
sudo insmod buddyAllocator.ko
sudo mknod /dev/char_dev c 100 0
sudo chmod a+w /dev/char_dev
dmesg
