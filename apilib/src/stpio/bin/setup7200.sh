echo "Setting up The 7200 environment for FK 2.6 linux"
echo "Please set KDIR and TOOLSET_ROOT environment variables"

# **********************************************************
# ********** SET ENVIRONMENT VARIABLES *********************
# **********************************************************

setenv DVD_ROOT /dvd-vob
setenv DVD_PLATFORM mb519

setenv DVD_SERVICE DVB
setenv DVD_FRONTEND 7200
setenv DVD_BACKTEND 7200

setenv SPECIAL_CONFIG_FILE mb519bypass

setenv DVD_MAKE $DVD_ROOT/dvdbr-prj-make

# This specifies where are the built objects are put.
setenv DVD_EXPORTS $DVD_ROOT/dvdbr-prj-stpio/objs

# The following is not defined if building in the vob
#setenv DVD_INCLUDE $DVD_ROOT/inc

setenv ARCHITECTURE ST40
setenv DVD_OS LINUX

# Add path to the toolset
#setenv TOOLSET_ROOT /opt/STM/STLinux-2.2/devkit/sh4
set path=($TOOLSET_ROOT/bin /opt/STM/ST40R4.0.1/bin $path)

# Add Linux kernel env
#setenv KDIR /opt/STM/STLinux-2.3ear/devkit/sources/kernel/linux-sh4_stm23ear_7200

setenv DVD_CFLAGS '-DST_OSLINUX -DARCHITECTURE_ST40 -DST_LINUX  -DDISABLE_TBX -DNO_STAPIGAT -DSTTBX_NO_UART -DSTPIO_LINUX_CORE_DEBUG'

#setenv DVD_BUILDING_IN_VOB true

echo "  --> KDIR = $KDIR"
echo "	--> Shell variables initialized."
