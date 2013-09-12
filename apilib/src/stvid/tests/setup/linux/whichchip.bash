#!/bin/bash
# File      : whichchip.sh
# Function  : Set variables according to the selected platform
# ************************************************************

answer_family="0"
while [ "$answer_family" -lt "1" -o "$answer_family" -gt "8" ] ; do
    echo
    echo "  Choose a chip family :"

    echo "  1) STi51xx family"
    echo "  2) STi53xx family"
    echo "  3) STi55xx family"
    echo "  4) STi70xx family"
    echo "  5) STi71xx family"
    echo "  6) STi72xx family"
    echo "  7) STi77xx family"
    echo "  8) STi80xx family"
    read answer_family

    case $answer_family in
        "1" )
        echo "Not available for Linux"
        ;;
        "2" )
        echo "Not available for Linux"
        ;;
        "3" )
        echo "Not available for Linux"
        ;;
        "4" )
        echo "Not available for Linux"
        ;;
        "5" )
        answer_chip="9"
        while [ "$answer_chip" -lt "0" -o "$answer_chip" -gt "2" ] ; do
            echo
            echo "  Choose a chip :"
            echo "  1) STi7100"
            echo "  2) STi7109"
            echo "  0) reset ..."
            read answer_chip
        done
        case $answer_chip in
            "0" )
            # return to family menu
            answer_family="0"
            ;;
            "1" )
            export DVD_FRONTEND="7100"
            export DVD_BACKEND="7100"
            export DVD_PLATFORM="mb411"
            source whichboard.bash "MENU7100"
#            export AUDIO_CELL="MMDSP"
#            export ATAPI_POLLING=1
            unset SATA_SUPPORTED
            export STVID_NO_HDD="1"
            export STVID_USE_UART_AS_STDIO=1
            # Multicom :
            export DVD_TOOLSET="LINUX"
            export MULTICOM=1
            export OS_0="linux"
            export OS_1="os21"
            export RPC_ROOT="$HOME/Linux/multicom"
            ;;
            "2" )
            export DVD_FRONTEND="7109"
            export DVD_BACKEND="7109"
            export DVD_PLATFORM="mb411"
            source whichboard.bash "MENU7109"
#            export AUDIO_CELL="MMDSP"
#            export ATAPI_POLLING=1
            # SATA drives in hdd.c not ported for 7109 yet
            unset SATA_SUPPORTED
            export STVID_NO_HDD="1"
            export STVID_USE_UART_AS_STDIO=1
            # Multicom :
            export DVD_TOOLSET="LINUX"
            export MULTICOM=1
            export OS_0="linux"
            export OS_1="os21"
            export RPC_ROOT="$HOME/Linux/multicom"
           ;;
        esac
        ;;
        "6" )
          export DVD_FRONTEND="7200"
          export DVD_BACKEND="7200"
          export DVD_PLATFORM="mb519"
          export BOARD="mb519"
#          export AUDIO_CELL="MMDSP"
#          export ATAPI_POLLING=1
	  export STVID_NO_HDD=1
	  #export STVID_USE_FILESYSTEM=1
          # SATA drives in hdd.c not ported for 7200 yet
          export  SATA_SUPPORTED=
          export  STVID_USE_UART_AS_STDIO=1
          # Multicom :
          export RPC_ROOT="$HOME/Linux/multicom"
          #export  MULTICOM_ROOT=%RPC_ROOT%
        ;;
        "7" )
          echo "Not available for Linux"
        ;;
        "8" )
          echo "Not available for Linux"
        ;;
        "9" )
          echo "Not available for Linux"
        ;;
    esac
# end while :
done

# ------ Conclusion
unset answer_family
unset answer_chip
echo   Setup ready for $DVD_BACKEND on $DVD_PLATFORM

