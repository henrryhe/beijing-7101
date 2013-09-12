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
setenv DVD_EXPORTS $DVD_ROOT/dvdbr-prj-stpwm/objs

# The following is not defined if building in the vob
#setenv DVD_INCLUDE $DVD_ROOT/inc

setenv ARCHITECTURE ST40
setenv DVD_OS LINUX

# Add path to the toolset
set path=($TOOLSET_ROOT/bin /opt/STM/ST40R4.0.1/bin $path)

# Add Linux kernel env

setenv DVD_CFLAGS -DSTPIO_LINUX_CORE_DEBUG 

echo "  --> KDIR = $KDIR"
echo "	--> Shell variables initialized."
