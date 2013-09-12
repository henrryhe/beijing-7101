#!/bin/sh
echo "Setting up The 7100 environment for FK 2.6 linux"
# **********************************************************
# ********** SET ENVIRONMENT VARIABLES *********************
# **********************************************************
setenv PATH /opt/STM/STLinux-2.2/devkit/sh4/bin/:$PATH
setenv PATH /opt/STM/ST40R4.0.1/bin/:$PATH
setenv TREE_ROOT /dvd-vob/
setenv DVD_ROOT $TREE_ROOT
setenv DVD_PLATFORM mb411
setenv DVD_TRANSPORT stpti4
setenv DVD_SERVICE DVB
setenv DVD_FRONTEND 7100
setenv DVD_BACKTEND 7100
setenv DVD_MAKE /dvd-vob/dvdbr-prj-make/
setenv DVD_EXPORTS /dvd-vob/dvdbr-prj-stblast/objs
setenv ARCHITECTURE ST40
setenv DVD_OS LINUX
setenv KDIR /opt/STM/STLinux-2.2/devkit/sources/kernel/local/linux-2.6.17.14_stm22_0037_intc2patch
setenv NO_STAPIGAT y
setenv DVD_CFLAGS '-DST_OSLINUX -DARCHITECTURE_ST40 -DST_LINUX -DLINUX_FULL_KERNEL_VERSION -DNO_STAPIGAT'
setenv MULTICOM 1
setenv LSPROOT /opt/STM/STLinux-2.2/devkit/sources/kernel/local/linux-2.6.17.14_stm22_0037_intc2patch
echo "  --> KDIR = $KDIR"
echo "	--> Shell variables are initialized."
