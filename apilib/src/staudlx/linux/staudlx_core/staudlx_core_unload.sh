#!/bin/sh
# invoke rmmod with all arguments we got

rmmod staudlx_core || exit 1


if [ "" == "${ST_DEV_ROOT_NAME}" ]
 then
    echo "Must define ST_DEV_ROOT_NAME"
    exit 0
fi

# Remove stale nodes
rm -f /dev/${ST_DEV_ROOT_NAME}/staudlx_core