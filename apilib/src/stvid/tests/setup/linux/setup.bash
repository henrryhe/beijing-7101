#!/bin/bash
# File       : setup.bash
# Function   : Set linux environment for video test application
#              as required for a release
# Assumption : run 'bash' first
#              then do 'source setup.bash'
# *************************************************************

echo
echo   Setup for STVID Release...
echo

# ------ Delete optionnal flags
unset DEBUG
unset DVD_CFLAGS
unset DVD_DEPENDS
unset DVD_INCLUDE
unset STVID_DCACHE_DISABLED
unset STVID_ICACHE_DISABLED
unset STVID_DEBUG_PRODUCER
unset STVID_DEBUG_PARSER
unset STVID_DEBUG_DECODER
unset STVID_TRACE_BUFFER_UART
unset MULTICOM_COMPANION_CONFIG
unset STCLKRV_CRYSTAL_ERROR

# Adds API to choose DEI mode
export VIDEO_DEBUG_DEINTERLACING_MODE=1
# Adds commands to get infos on displayed picture
export STVID_DEBUG_GET_DISPLAY_PARAMS=1

# ------ Set mandatory variables
export DVD_STVIN="MASTER"
export MAKE_MODE="UNIX"
#export GCC_CHECK=1
# Use Posix function for keyboard input
#SET DVD_POSIX=1
export STVID_DEBUG_GET_STATISTICS=1
export STVID_USE_FILESYSTEM=1
export STVID_DIRECT_INTERFACE_PTI=1

# ------ Choose and export a platform
source whichchip.bash
if [ "$DVD_BACKEND" == "" ] ; then
  echo "DVD_BACKEND not defined !"
  exit 0
fi

# ------ Choose DCU Toolset

source whichdcu.bash

# Set custom setup, like personnal working directory
source mytargetsetup.bash

export DVD_EXPORTS="$MY_ROOTFS_DIR/exports"
export ROOTFS_DIR="$MY_ROOTFS_DIR"

# ------ Choose the firmware

if [ "$DVD_BACKEND" == "7100" ] ; then
    source whichfirmware.bash
fi
if [ "$DVD_BACKEND" == "7109" ] ; then
    source whichfirmware.bash
fi
if [ "$DVD_BACKEND" == "7200" ] ; then
    source whichfirmware.bash
fi

# ------ export the PTI

export DVD_TRANSPORT="STPTI4"
echo
echo   DVD_TRANSPORT export to $DVD_TRANSPORT

# ------ Choose the SERVICE

export DVD_SERVICE="DVB"
echo   SERVICE export to $DVD_SERVICE

# ------ Choose the MEMORY

unset UNIFIED_MEMORY
export DVD_CFLAGS="$DVD_CFLAGS -DSTAVMEM_DEBUG_MEMORY_STATUS"

# ------ Set the View

export DVD_ROOT="/vob"
export DVD_MAKE="$DVD_ROOT/dvdbr-prj-make"

# ------ Display the variables
echo
#set | sort

# ------ Conclusion
echo   Setup ready for $DVD_BACKEND on $DVD_PLATFORM

