#!/bin/bash
# File     : whichdcu.bash
# Function : Select which DCU must be used
# ****************************************

answer_linux="0"
answer_platform="0"

while [ "$answer_linux" -lt "1" -o "$answer_linux" -gt "4" ] ; do
    echo
    echo "  Choose a DCU Toolset :"
    echo "  1) Linux 710x LDDE2.0"
    echo "  2) Linux 710x LDDE2.2"
    echo "  3) Linux 7200 LDDE2.3ear"
    echo "  4) Linux LDDE2.3"
    read answer_linux

    case $answer_linux in
        "1" )
        export LINUX_DISTRIB="/opt/STM/STLinux-2.0"
        export LINUX_SH4="$LINUX_DISTRIB/devkit/sh4"
        KERNEL_VERSION=2.0
        BIT_DEPTH="29bits"

        echo "Set path..."
        export PATH="/opt/STM/st40load/bin:/usr/X11R6/bin:/bin:/usr/bin:/sw/st/util/local/bin"
        export PATH="$PATH:/sw/st/util/common/bin:/apa/util/bin:/usr/atria/bin:/bin:/usr/local/sbin:/usr/local/bin:/sbin"
        export PATH="$PATH:$HOME/bin:$DVD_ROOT/dvdbr-admin/scripts/user:$HOME/Linux/multicom/bin/linux/ia32:."
        ;;

        "2" )
        export LINUX_DISTRIB="/opt/STM/STLinux-2.2"
        export LINUX_SH4="$LINUX_DISTRIB/devkit/sh4"
        KERNEL_VERSION=2.2
        BIT_DEPTH="29bits"

        echo "Set path..."
        export PATH="$LINUX_SH4/bin:/usr/X11R6/bin:/bin:/usr/bin:/sw/st/util/local/bin"
        export PATH="$PATH:/sw/st/util/common/bin:/apa/util/bin:/usr/atria/bin:/bin:/usr/local/sbin:/usr/local/bin:/sbin"
        export PATH="$PATH:$HOME/bin:$DVD_ROOT/dvdbr-admin/scripts/user:$HOME/Linux/multicom/bin/linux/ia32:."
        ;;

        "3" )
        # KDIR is defined using MY_KDIR variable in the mytargetsetup.bash
        export LINUX_DISTRIB="/opt/STM/STLinux-2.3ear"
        export LINUX_SH4="$LINUX_DISTRIB/devkit/sh4"
        KERNEL_VERSION=2.3ear
        BIT_DEPTH="29bits"

        echo "Set path..."
        export PATH="$LINUX_SH4/bin:/usr/X11R6/bin:/bin:/usr/bin:/sw/st/util/local/bin"
        export PATH="$PATH:/sw/st/util/common/bin:/apa/util/bin:/usr/atria/bin:/bin:/usr/local/sbin:/usr/local/bin:/sbin"
        export PATH="$PATH:$HOME/bin:$DVD_ROOT/dvdbr-admin/scripts/user:$HOME/Linux/multicom/bin/linux/ia32:."
        ;;

        "4" )
        # KDIR is defined using MY_KDIR variable in the mytargetsetup.bash
        export LINUX_DISTRIB="/opt/STM/STLinux-2.3"
        export LINUX_SH4="$LINUX_DISTRIB/devkit/sh4"
        KERNEL_VERSION=2.3

        while [ "$answer_platform" -lt "1" -o "$answer_platform" -gt "2" ] ; do
            echo
            echo "  Bit depth (default: 29 bits)"
            echo "  1) 29-bit"
            echo "  2) 32-bit"
            read answer_platform
            case $answer_platform in
                "2" )
                    BIT_DEPTH="32bits"
                    ;;
                * )
                    BIT_DEPTH="29bits"
                    ;;
            esac
        done

        echo "Set path..."
        export PATH="$LINUX_SH4/bin:/usr/X11R6/bin:/bin:/usr/bin:/sw/st/util/local/bin"
        export PATH="$PATH:/sw/st/util/common/bin:/apa/util/bin:/usr/atria/bin:/bin:/usr/local/sbin:/usr/local/bin:/sbin"
        export PATH="$PATH:$HOME/bin:$DVD_ROOT/dvdbr-admin/scripts/user:$HOME/Linux/multicom/bin/linux/ia32:."
        ;;
    esac

    if   [ "$DVD_BACKEND" = "7100" ] ; then
        FINAL_CONFIG="$KERNEL_VERSION"_710x_"$BIT_DEPTH"_"$BOARD"
    elif [ "$DVD_BACKEND" = "7109" ] ; then
        FINAL_CONFIG="$KERNEL_VERSION"_710x_"$BIT_DEPTH"_"$BOARD"
    else
        FINAL_CONFIG="$KERNEL_VERSION"_"$DVD_BACKEND"_"$BIT_DEPTH"_"$BOARD"
    fi

    echo "Set variables for sh4 linux: $DVD_BACKEND $BIT_DEPTH for $BOARD on kernel LDDE $KERNEL_VERSION ..."
    case $FINAL_CONFIG in
        "2.0_710x_29bits_mb411" )
            export KDIR="$LINUX_DISTRIB/devkit/kernels/linux-sh4-2.6.11.12_stm20-32-710x-mb411-ref"
    
            echo "Update multicom and stdcmd..."
            rm -f ~/Linux/multicom ~/Linux/stdcmd
            ln -s $LINUX_SH4/stdcmd       ~/Linux/stdcmd
            ln -s $LINUX_DISTRIB/devkit/Multicom/multicom-3.1.2-LDDE2.0-710x-mb411   ~/Linux/multicom
            ;;
        
        "2.2_710x_29bits_mb411" )
            export KDIR="$LINUX_DISTRIB/devkit/kernels/linux-2.6.17.14_stm22_0036-710x-mb411-ref"

            echo "Update multicom & stdcmd ..."
            rm -f ~/Linux/multicom ~/Linux/stdcmd
            ln -s $LINUX_SH4/stdcmd       ~/Linux/stdcmd
            ln -s $LINUX_DISTRIB/devkit/Multicom/multicom-3.1.2-LDDE2.2-710x-mb411   ~/Linux/multicom
            ;;
        
        "2.2_710x_29bits_coco" )
            export KDIR="$LINUX_DISTRIB/devkit/kernels/linux-2.6.17.14_stm22_0036-710x-coco-ref"

            echo "Update multicom & stdcmd ..."
            rm -f ~/Linux/multicom ~/Linux/stdcmd
            ln -s $LINUX_SH4/stdcmd       ~/Linux/stdcmd
            ln -s $LINUX_DISTRIB/devkit/Multicom/multicom-3.1.2-LDDE2.2-710x-coco   ~/Linux/multicom
            ;;
        
        "2.3ear_7200_29bits_mb519" )
            export KDIR="$LINUX_DISTRIB/devkit/kernels/linux-sh4_stm23ear-7200-29bits-ref"
            
            echo "Update multicom and stdcmd..."
            rm -f ~/Linux/multicom ~/Linux/stdcmd
            ln -s $LINUX_SH4/stdcmd       ~/Linux/stdcmd
            ln -s $LINUX_DISTRIB/devkit/Multicom/multicom-3.1.2-LDDE2.3ear-7200-29bits    ~/Linux/multicom
            ;;

        "2.3_710x_29bits_mb411" )
            export KDIR="$LINUX_DISTRIB/devkit/kernels/linux-sh4-2.6.23.1_stm23_0102-710x-29bits-mb411-ref"
            
            echo "Update multicom ..."
            rm -f ~/Linux/multicom ~/Linux/stdcmd
            ln -s $LINUX_SH4/stdcmd       ~/Linux/stdcmd
            ln -s $LINUX_DISTRIB/devkit/Multicom/multicom-3.1.2-LDDE2.3-710x-29bits-mb411    ~/Linux/multicom
            ;;

        "2.3_710x_32bits_mb411" )
            export KDIR="$LINUX_DISTRIB/devkit/kernels/linux-sh4-2.6.23.1_stm23_0102-710x-32bits-mb411-ref"

            echo "Update stdcmd..."
            rm -f ~/Linux/stdcmd
            ln -s $LINUX_SH4/stdcmd                 ~/Linux/stdcmd

            echo "Update multicom ..."
            rm -f ~/Linux/multicom
            echo;echo;echo
            echo "WARNING !!!! : NO MULTICOM available with this kernel !!!"
            echo "               Please use your own compiled multicom"
            echo; echo
            echo "Press Enter ..."
            read answer_platform
            ;;

        "2.3_7200_29bits_mb519" )
            export KDIR="$LINUX_DISTRIB/devkit/kernels/linux-sh4-2.6.23.1_stm23_0102-7200-29bits-ref"

            echo "Update stdcmd..."
            rm -f ~/Linux/stdcmd
            ln -s $LINUX_SH4/stdcmd                 ~/Linux/stdcmd

            echo "Update multicom ..."
            rm -f ~/Linux/multicom
            echo;echo;echo
            echo "WARNING !!!! : NO MULTICOM available with this kernel !!!"
            echo "               Please use your own compiled multicom"
            echo; echo
            echo "Press Enter ..."
            read answer_platform
            ;;

        "2.3_7200_32bits_mb519" )
            export KDIR="$LINUX_DISTRIB/devkit/kernels/linux-sh4-2.6.23.1_stm23_0102-7200-32bits-ref"

            echo "Update stdcmd..."
            rm -f ~/Linux/stdcmd
            ln -s $LINUX_SH4/stdcmd                 ~/Linux/stdcmd

            echo "Update multicom ..."
            rm -f ~/Linux/multicom
            echo;echo;echo
            echo "WARNING !!!! : NO MULTICOM available with this kernel !!!"
            echo "               Please use your own compiled multicom"
            echo; echo
            echo "Press Enter ..."
            read answer_platform
            ;;
            
        * )
            export KDIR=""
            echo;echo;echo
            echo "WARNING !!!! : NO KERNEL available in this configuration !!!"
            echo "               Please use your own compiled kernel"
            echo; echo
            echo "Press Enter ..."
            read answer_platform

            echo "Update stdcmd..."
            rm -f ~/Linux/stdcmd
            ln -s $LINUX_SH4/stdcmd                 ~/Linux/stdcmd

            echo "Update multicom ..."
            rm -f ~/Linux/multicom
            echo;echo;echo
            echo "WARNING !!!! : NO MULTICOM available !!!"
            echo "               Please use your own compiled multicom"
            echo; echo
            echo "Press Enter ..."
            read answer_platform
            ;;        
    esac
    
    
    export ARCH="sh"
    export ARCHITECTURE="ST40"
    export CROSS_COMPILE="sh4-linux-"
    export DVD_OS="LINUX"
    export KERNELDIR=$KDIR
    export LINUX_ARCHITECTURE="LINUX_FULL_KERNEL_VERSION"
    export LSPROOT=$KDIR

done

unset answer_linux
unset answer_platform

