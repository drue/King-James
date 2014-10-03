#!/bin/sh

TARGET="${1}"

# copy System.map
cp ${TARGET}/../build/linux-*/System.map ${TARGET}/System.map

# copy kernel
cp ${TARGET}/../images/zImage ${TARGET}/../images/boot/kernel.img

# config
cp /james/board/config.txt ${TARGET}/../images/boot/config.txt

exit 0
