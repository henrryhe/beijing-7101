#!/bin/sh

echo "Setting up The 7109 environment for 2.6 linux"

# **********************************************************
# ********** SET ENVIRONMENT VARIABLES *********************
# **********************************************************
export PATH=/opt/STM/ST40R4.0.1/bin:/opt/STM/STLinux-2.2/devkit/sh4/bin:$PATH
export DEVPATH=/dvd-vob
export ROOTFS_DIR=/opt/STM/STLinux-2.2/devkit/sh4/target_mb411
export DVD_ROOT=/dvd-vob
export DVD_MAKE=$DVD_ROOT/dvdbr-prj-make
#export DVD_INCLUDE="$DVD_ROOT/dvdbr-prj-include"
export ARCHITECTURE=ST40
export DVD_OS=LINUX
export DVD_TOOLSET=LINUX
export DVD_EXPORTS=/dvd-vob/dvdbr-prj-stsmart
export DVD_FRONTEND=7109
export DVD_BACKEND=7109
export DVD_PLATFORM=mb411
export DVD_SERVICE=DVB
export DVD_LAYERAPI=TRUE

export LINUX_ARCHITECTURE=LINUX_FULL_KERNEL_VERSION

#specificity to 2.6 kernel
export LINUX_KERNEL_VERSION=2.6
export COMPILATION_26=1
export ARCH=sh
export CROSS_COMPILE=sh4-linux-

#export KERNEL_RELEASE_NAME=/opt/STM/STLinux-2.2/devkit/sources/kernel/local/linux-2.6.17.14_stm22_0036
#export KDIR=/opt/STM/STLinux-2.2/devkit/sources/kernel/local/linux-2.6.17.14_stm22_0036

export KERNEL_RELEASE_NAME=/opt/STM/STLinux-2.2/devkit/sources/kernel/local/linux-2.6.17.14_stm22_0037_intc2patch
export KDIR=/opt/STM/STLinux-2.2/devkit/sources/kernel/local/linux-2.6.17.14_stm22_0037_intc2patch

##### FOR ECHOSTAR ####
#export KERNEL_RELEASE_NAME=/data/hvd7109eco/aakashr/echostarsupport/vobs/st/os/linux/linux-2.6.17
#export KDIR=/data/hvd7109eco/aakashr/echostarsupport/vobs/st/os/linux/linux-2.6.17

export DVD_CFLAGS='-I/dvd-vob/dvdbr-prj-stcommon -I/dvd-vob/dvdbr-prj-stpio -I/dvd-vob/dvdca-prj-stevt -DUART_BAUD_RATE=115200 -DDVD_DEBUG_DISP_REG_CONTROL -DLINUX_OS -DST_OSLINUX -DUSE_LMI_SYS_FOR_LINUX -DSTVID_DEBUG_GET_STATISTICS -DGLOBAL_PASS_PARAMS_THROUGH_IOCTLS -DSTLINUX_RT_THREADS -DSTAPI_LINUX_DEV_DIR="\"/vob/\"" -DLINUX_FULL_KERNEL_VERSION -pthread -DAVMEM_KERNEL_MODE -DSTTBX_PRINT -DDISABLE_TBX'

export DVD_BUILDING_IN_VOB=true
#export STAVMEM_NO_FDMA=1

# MULTICOM specific
export MULTICOM=1

#export LSPROOT=/opt/STM/STLinux-2.2/devkit/sources/kernel/local/linux-2.6.17.14_stm22_0036/
export LSPROOT=/opt/STM/STLinux-2.2/devkit/sources/kernel/local/linux-2.6.17.14_stm22_0037_intc2patch/

##### FOR ECHOSTAR ####
#export LSPROOT=/data/hvd7109eco/aakashr/echostarsupport/vobs/st/os/linux/linux-2.6.17/


export RPC_ROOT=/opt/STM/multicom-3.0.4
export MULTICOM_BASE=/opt/STM/multicom-3.0.4
#export DVD_CFLAGS="-DST_OS21_TO_LINUX_DEFS=\"compat.h\" -DDVD_EXCLUDEOS20TO21_MACROS -DMODULE_LIB $DVD_CFLAGS"
export MV=mv
export CP=cp

echo "	--> Shell variables initialized."

