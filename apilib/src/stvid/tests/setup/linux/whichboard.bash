#!/bin/bash
# File      : whichboard.sh
# Function  : Set variables according to the selected board
# Parameter : optional $1=MENU5518 MENU5528 MENU7020 MENUOMEGA
#                         MENU5100
# *************************************************************

echo
echo "  Choose a board :"
case $1 in
    "MENU5100" )
    echo "  1) MB390 (validation board = default)"
    echo "  2) MB395 (reference platform)"
    echo "  3) MB424 (IP set-top box with Ethernet)"
    read answer_board
    echo "not available"
    ;;
    "MENU5518" )
    echo "  1) MB275 rev A (32Mb)"
    echo "  2) MB275 rev B (64Mb)"
    echo "  3) MB5518 Force"
    read answer_board
    echo "not available"
    ;;
    "MENU5528" )
    echo "  1) MB376"
    echo "  2) espresso"
    read answer_board
    echo "not available"
    ;;
    "MENU7100" | "MENU7109" )
    answer_board="0"
    while [ "$answer_board" \< "1" ] || [ "$answer_board" \> "2" ] ; do
        unset BOARD
        echo "  1) MB411 - osc. 27 Mhz  (default)"
        echo "  2) REF7100 COCO - osc. 30 Mhz"
        read answer_board
        case $answer_board in
            "2" )
            export DVD_PLATFORM="mb411"
            export BOARD="coco"
            # Temporary flags used for link, while there is no official pti release, and WAs for clkrv
            export DVD_CFLAGS="-DBOARD71XX_WA_OSC_VALUE=30"
            export STCLKRV_CRYSTAL_ERROR=
            ;;
            * )
            answer_board="1"
            export DVD_PLATFORM="mb411"
            export BOARD="mb411"
            # Temporary flags used for link, while there is no official pti release, and WAs for clkrv
            export DVD_CFLAGS="-DBOARD71XX_WA_OSC_VALUE=27"
            export STCLKRV_CRYSTAL_ERROR="7000"
            ;;
        esac
    done
    ;;
    "MENU7020" )
    echo "  1) MB295 (Eval 7015 or 7020)"
    echo "  2) MB290 (ATV2 7020+5514)"
    echo "  3) MB382 (STEM 7020+5517)"
    read answer_board
    echo "not available"
    ;;
    "MENUOMEGA" )
    echo "  1) Omega1 : 5517 only"
    echo "  2) Omega2 : 5517 + STEM 7020"
    read answer_board
    echo "not available"
    ;;
esac

# ------ Conclusion
unset answer_board

