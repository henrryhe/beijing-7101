o "Setting up The 7100 environment for FK 2.6 linux"

# **********************************************************
# ********** SET ENVIRONMENT VARIABLES *********************
# **********************************************************
setenv  PATH /opt/STM/STLinux-2.2/devkit/sh4/bin:/opt/STM/ST40R4.0.1/bin:$PATH

setenv TREE_ROOT /dvd-vob
setenv DVD_PLATFORM mb411
setenv DVD_TRANSPORT stpti4
setenv DVD_SERVICE DVB
setenv DVD_FRONTEND 7100
setenv DVD_BACKTEND 7100

setenv DVD_MAKE $TREE_ROOT/dvdbr-prj-make
setenv DVD_ROOT $TREE_ROOT
# This specifies where are the built objects are put.
setenv DVD_EXPORTS $TREE_ROOT/dvdbr-prj-stpio/objs
# The following is not defined if building in the vob
#setenv DVD_INCLUDE $TREE_ROOT/inc

setenv ARCHITECTURE ST40
setenv DVD_OS LINUX

# Add path to the toolset
setenv TOOLSET_ROOT /opt/STM/STLinux-2.2/devkit/sh4
set path=($path $TOOLSET_ROOT/bin)
# Add Linux kernel env

setenv  KDIR /opt/STM/STLinux-2.2/devkit/sources/kernel/linux-2.6.17.14_stm22_0036_pio
#setenv KDIR ~/linux/kernel/linux-sh4-2.6.11_stm20-30
#setenv KDIR /opt/STM/STLinux-2.0/devkit/kernel/linux-sh4-2.6.11.12_stm20-32
#setenv KDIR /opt/STM/STLinux-2.2ear/devkit/kernel/linux-sh4-2.6.17.8_stm22ear-34

#setenv ROOTFS_DIR /opt/STM/STLinux-2.0/devkit/sh4/target_mb411
#setenv KERNEL_RELEASE_NAME /opt/STM/STLinux-2.0/devkit/kernel/linux-sh4-2.6.11_stm20-30

setenv DVD_CFLAGS '-DST_OSLINUX -DARCHITECTURE_ST40 -DST_LINUX  -DDISABLE_TBX -DNO_STAPIGAT -DSTTBX_NO_UART -DSTPIO_LINUX_CORE_DEBUG'

#setenv DVD_BUILDING_IN_VOB true

# MULTICOM specific
setenv MULTICOM 1
setenv  LSPROOT /opt/STM/STLinux-2.2/devkit/sources/kernel/linux-2.6.17.14_stm22_0036_pio/
#setenv LSPROOT /opt/STM/STLinux-2.2ear/devkit/kernel/linux-sh4-2.6.17.8_stm22ear-34/
#setenv LSPROOT /opt/STM/STLinux-2.0/devkit/kernel/linux-sh4-2.6.11.12_stm20-32/
#setenv LSPROOT /opt/STM/STLinux-2.0/devkit/kernel/linux-sh4-2.6.11_stm20-31/
setenv RPC_ROOT /opt/STM/multicom-3.0.4
setenv MULTICOM_BASE /opt/STM/multicom-3.0.4
#setenv DVD_CFLAGS "-DST_OS21_TO_LINUX_DEFS=compat.h -DDVD_EXCLUDEOS20TO21_MACROS -DMODULE_LIB $DVD_CFLAGS"
#setenv MV mv
#setenv CP cp

echo "  --> KDIR = $KDIR"
echo "	--> Shell variables initialized."


