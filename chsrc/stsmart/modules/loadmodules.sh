#!/bin/sh
# Load the driver in the root directory
if [ "" != "$1" ]
then
    root="$1"
else
    root="."
fi

if [ "" == "${ST_DEV_ROOT_NAME}" ]
 then
    echo "Must define ST_DEV_ROOT_NAME"
    exit 0
fi

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default

#insmod -f ${root}/../stpio/stpio.ko || exit 1
insmod -f ${root}/stsmart_core.ko || exit 1

major=`cat /proc/devices | awk "\\$2==\"stsmart_core\" {print \\$1}"`
# Remove stale nodes and replace them, then give gid and perms
# Usually the script is shorter, it's scull that has several devices in it.

rm -f /dev/${ST_DEV_ROOT_NAME}/stsmart_core

mknod /dev/${ST_DEV_ROOT_NAME}/stsmart_core c $major 0     # 00

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
insmod -f ${root}/stsmart_ioctl.ko || exit 1

major=`cat /proc/devices | awk "\\$2==\"stsmart_ioctl\" {print \\$1}"`
# Remove stale nodes and replace them, then give gid and perms
# Usually the script is shorter, it's scull that has several devices in it.

rm -f /dev/${ST_DEV_ROOT_NAME}/stsmart_ioctl

mknod /dev/${ST_DEV_ROOT_NAME}/stsmart_ioctl c $major 0     # 00
