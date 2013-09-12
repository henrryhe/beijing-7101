o "Setting up The 7200 environment for FK 2.6 linux"

# **********************************************************
# ********** SET ENVIRONMENT VARIABLES *********************
# **********************************************************
setenv  PATH /local_no_backup/gaurav/opt/STM/STLinux-2.3ear/devkit/sh4/bin:/opt/STM/ST40R4.0.1/bin:$PATH

#export ROOTFS_DIR=/opt/STM/STLinux-2.2/devkit/sh4/target_mb411

#specificity to 2.6 kernel
setenv LINUX_KERNEL_VERSION 2.6
setenv COMPILATION_26 1
setenv ARCH sh
setenv CROSS_COMPILE sh4-linux-
setenv DEVPATH /dvd-vob
setenv DVD_ROOT /dvd-vob
setenv DVD_MAKE $DVD_ROOT/dvdbr-prj-make
setenv ARCHITECTURE ST40
setenv DVD_OS LINUX
setenv DVD_TOOLSET LINUX
setenv DVD_EXPORTS /dvd-vob/dvdbr-prj-stsmart
setenv DVD_PLATFORM mb519
setenv DVD_SERVICE DVB
setenv DVD_FRONTEND 7200
setenv DVD_BACKTEND 7200
setenv SPECIAL_CONFIG_FILE mb519bypass
setenv DVD_LAYERAPI TRUE
setenv LINUX_ARCHITECTURE LINUX_FULL_KERNEL_VERSION

# Add path to the toolset
setenv TOOLSET_ROOT /local_no_backup/gaurav/opt/STM/STLinux-2.2/devkit/sh4
set path=($path $TOOLSET_ROOT/bin)

# Add Linux kernel env
setenv KERNEL_RELEASE_NAME /local_no_backup/gaurav/opt/STM/STLinux-2.3ear/devkit/sources/kernel/linux-sh4_stm23ear_7200
setenv KDIR /local_no_backup/gaurav/opt/STM/STLinux-2.3ear/devkit/sources/kernel/linux-sh4_stm23ear_7200
setenv DVD_CFLAGS '-I/dvd-vob/dvdbr-prj-stcommon -I/dvd-vob/dvdbr-prj-stpio -I/dvd-vob/dvdca-prj-stevt -DUART_BAUD_RATE=115200 -DDVD_DEBUG_DISP_REG_CONTROL -DLINUX_OS -DST_OSLINUX -DUSE_LMI_SYS_FOR_LINUX -DSTVID_DEBUG_GET_STATISTICS -DGLOBAL_PASS_PARAMS_THROUGH_IOCTLS -DSTLINUX_RT_THREADS -DSTAPI_LINUX_DEV_DIR="\"/vob/\"" -DLINUX_FULL_KERNEL_VERSION -pthread -DAVMEM_KERNEL_MODE -DSTTBX_PRINT -DDISABLE_TBX'

# MULTICOM specific
setenv MULTICOM 1
setenv LSPROOT /local_no_backup/gaurav/opt/STM/STLinux-2.3ear/devkit/sources/kernel/linux-sh4_stm23ear_7200/
setenv RPC_ROOT /opt/STM/multicom-3.0.4
setenv MULTICOM_BASE /opt/STM/multicom-3.0.4


echo "  --> KDIR = $KDIR"
echo "	--> Shell variables initialized."


