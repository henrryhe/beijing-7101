#!/bin/sh
# invoke rmmod with all arguments we got

echo "Removing modules..."
lsmod
if [ "" == "${ST_DEV_ROOT_NAME}" ]
 then
    echo "Must define ST_DEV_ROOT_NAME"
    exit 0
fi

# Remove stale nodes

rmmod sttuner_ioctl || exit 1

rm -f /dev/${ST_DEV_ROOT_NAME}/sttuner_ioctl

rmmod sttuner_core || exit 1

rm -f /dev/${ST_DEV_ROOT_NAME}/sttuner_core

rmmod stevt_ioctl || exit 1

rm -f /dev/${ST_DEV_ROOT_NAME}/stevt_ioctl

rmmod stevt_core || exit 1

rm -f /dev/${ST_DEV_ROOT_NAME}/stevt_core

rmmod sti2c_ioctl || exit 1

rm -f /dev/${ST_DEV_ROOT_NAME}/sti2c_ioctl

rmmod sti2c_core || exit 1

rmmod stcommon_core || exit 1

rmmod stos_core || exit 1

echo "Modules unloaded..."
lsmod

