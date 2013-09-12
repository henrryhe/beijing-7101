#!/bin/sh

date -s 'Tue, 20 Dec 2005 11:31:47 +0200'
ST_DEV_ROOT_NAME=stapi
export ST_DEV_ROOT_NAME
STEVT_IOCTL_DEV_PATH="/dev/stapi/stevt_ioctl"
export STEVT_IOCTL_DEV_PATH
STTUNER_IOCTL_DEV_PATH=/dev/${ST_DEV_ROOT_NAME}/sttuner_ioctl
export STTUNER_IOCTL_DEV_PATH
STI2C_IOCTL_DEV_PATH="/dev/${ST_DEV_ROOT_NAME}/sti2c_ioctl"
STI2C_IOCTL_BUS_PATH="/dev/i2c-0"
export STI2C_IOCTL_DEV_PATH STI2C_IOCTL_BUS_PATH


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

# Load the driver in the root directory

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
insmod -f ${root}/exports/lib/modules/stos_core.ko || exit 1
insmod -f ${root}/exports/lib/modules/stcommon_core.ko || exit 1
insmod -f ${root}/exports/lib/modules/sti2c_core.ko || exit 1
insmod -f ${root}/exports/lib/modules/sti2c_ioctl.ko || exit 1

major=`cat /proc/devices | awk "\\$2==\"sti2c_ioctl\" {print \\$1}"`
# Remove stale nodes and replace them, then give gid and perms
# Usually the script is shorter, it's scull that has several devices in it.

rm -f /dev/${ST_DEV_ROOT_NAME}/sti2c_ioctl

mknod /dev/${ST_DEV_ROOT_NAME}/sti2c_ioctl c $major 0     # 00

# Load the driver in the root directory

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
insmod -f ${root}/exports/lib/modules/stevt_core.ko || exit 1

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
insmod -f ${root}/exports/lib/modules/stevt_ioctl.ko || exit 1

major=`cat /proc/devices | awk "\\$2==\"stevt_ioctl\" {print\\$1}"`
rm -f /dev/${ST_DEV_ROOT_NAME}/stevt_ioctl

mknod /dev/${ST_DEV_ROOT_NAME}/stevt_ioctl c $major 0 #00

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
insmod -f ${root}/exports/lib/modules/sttuner_core.ko || exit 1

major=`cat /proc/devices | awk "\\$2==\"sttuner_core\" {print \\$1}"`
# Remove stale nodes and replace them, then give gid and perms
# Usually the script is shorter, it's scull that has several devices in it.

rm -f /dev/${ST_DEV_ROOT_NAME}/sttuner_core

mknod /dev/${ST_DEV_ROOT_NAME}/sttuner_core c $major 0     # 00

# Load the driver in the root directory

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
insmod -f ${root}/exports/lib/modules/sttuner_ioctl.ko || exit 1

major=`cat /proc/devices | awk "\\$2==\"sttuner_ioctl\" {print \\$1}"`
# Remove stale nodes and replace them, then give gid and perms
# Usually the script is shorter, it's scull that has several devices in it.

rm -f /dev/${ST_DEV_ROOT_NAME}/sttuner_ioctl

mknod /dev/${ST_DEV_ROOT_NAME}/sttuner_ioctl c $major 0     # 00
