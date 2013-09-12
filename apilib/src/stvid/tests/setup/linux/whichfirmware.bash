#!/bin/bash
# File       : whichfirmware.sh
# Function   : Set variables according to the selected platform
#              for future use in scripts 'loadimage' and 'loadmods'
# Assumption : ROOTFS_DIR defined
# *****************************************************************

COMPANION_DIR=scripts/vid

answer_is_bad=1
answer_firmware="0"
while [ $answer_is_bad -eq 1 ] ; do
    echo
    echo "  Choose a firmware :"

    case $DVD_BACKEND in
        # ........................................................
        "7100" )
            echo "  1) VideoLX_7100BxCx_H264_AV_0x04000000.bin + AudioLX_0x04100000.bin" 
            echo "  2) VideoLX_7100BxCx_H264_MPEG4P2_AV_0x04000000.bin + AudioLX_0x04200000.bin"
            echo "  3) VideoLX_7100BxCx_MPEG4P2_AV_0x04000000.bin + AudioLX_0x04200000.bin"
           echo " Enter your choice (default is 1)"
            read answer_firmware
            case $answer_firmware in
               "2" )
               COMPANION_VIDEO=$COMPANION_DIR/VideoLX_7100BxCx_H264_MPEG4P2_AV_0x04000000.bin
               COMPANION_AUDIO=$COMPANION_DIR/AudioLX_0x04200000.bin
               answer_is_bad=0
               ;;
               "3" )
               COMPANION_VIDEO=$COMPANION_DIR/VideoLX_7100BxCx_MPEG4P2_AV_0x04000000.bin
               COMPANION_AUDIO=$COMPANION_DIR/AudioLX_0x04200000.bin
               answer_is_bad=0
               ;;
               * )
               COMPANION_VIDEO=$COMPANION_DIR/VideoLX_7100BxCx_H264_AV_0x04000000.bin
               COMPANION_AUDIO=$COMPANION_DIR/AudioLX_0x04100000.bin
               answer_is_bad=0
               ;;
           esac
        ;;
        # ........................................................
        "7109" )
           echo "  1) VideoLX_7109C0_H264_VC1_AV_0x04000000.bin + AudioLX_0x04200000.bin"
           echo "  2) VideoLX_7109C0_MPEG4P2_AV_0x04000000.bin + AudioLX_0x04200000.bin"
           echo "  3) VideoLX_7109C0_H264_VC1_MPEG4P2_AV_0x04000000.bin + AudioLX_0x04300000.bin"
           echo "  4) VideoLX_7109C0_H264_AV_0x04000000.bin + AudioLX_0x04100000.bin"
           echo "  5) VideoLX_7109C0_H264_MPEG4P2_AV_0x04000000.bin + AudioLX_0x04200000.bin"
           echo "  Enter your choice (default is 1)"
           read answer_firmware
           case $answer_firmware in
               "2" )
               COMPANION_VIDEO=$COMPANION_DIR/VideoLX_7109C0_MPEG4P2_AV_0x04000000.bin
               COMPANION_AUDIO=$COMPANION_DIR/AudioLX_0x04200000.bin
               answer_is_bad=0
               ;;
               "3" )
               echo "  WARNING!: check that memory map used to compile allow to try this config"
               COMPANION_VIDEO=$COMPANION_DIR/VideoLX_7109C0_H264_VC1_MPEG4P2_AV_0x04000000.bin
               COMPANION_AUDIO=$COMPANION_DIR/AudioLX_0x04300000.bin
               answer_is_bad=0
               ;;
               "4" )
               COMPANION_VIDEO=$COMPANION_DIR/VideoLX_7109C0_H264_AV_0x04000000.bin
               COMPANION_AUDIO=$COMPANION_DIR/AudioLX_0x04100000.bin
               answer_is_bad=0
               ;;
               "5" )
               COMPANION_VIDEO=$COMPANION_DIR/VideoLX_7109C0_H264_MPEG4P2_AV_0x04000000.bin
               COMPANION_AUDIO=$COMPANION_DIR/AudioLX_0x04200000.bin
               answer_is_bad=0
               ;;
               * )
               COMPANION_VIDEO=$COMPANION_DIR/VideoLX_7109C0_H264_VC1_AV_0x04000000.bin
               COMPANION_AUDIO=$COMPANION_DIR/AudioLX_0x04200000.bin
               answer_is_bad=0
               ;;
           esac
        ;;
        # ........................................................
        "7200" )
           echo "  1) VideoLX_7200A0V1_H264_VC1_MPEG2_MPEG4P2_V1_0x08000000.bin"
           echo "  + VideoLX_7200A0V2_H264_VC1_MPEG2_MPEG4P2_V2_0x08300000.bin"
#           echo "  2) VideoLX_7200A0V1_H264_VC1_MPEG2_MPEG4P2_V1_0x08000000.bin"
#	   echo "    + VideoLX_7200A0V2_H264_VC1_MPEG2_MPEG4P2_V2_0x08300000.bin"
#           echo "     AudioLX_0x04200000.bin"
#           echo "     AudioLX_0x04300000.bin"
           echo "  Enter your choice (default is 1)"
           read answer_firmware
           case $answer_firmware in
 #              "2" )
	       # Set multicom companion variable
#	       export MULTICOM_COMPANION_CONFIG=0x1F

	       # Set audio firmwares
#               COMPANION_VIDEO1=$COMPANION_DIR/VideoLX_7200A0V1_H264_VC1_MPEG2_MPEG4P2_V1_0x08000000.bin
#               COMPANION_VIDEO2=$COMPANION_DIR/VideoLX_7200A0V2_H264_VC1_MPEG2_MPEG4P2_V2_0x08300000.bin
#               COMPANION_AUDIO1=$COMPANION_DIR/AudioLX_0x04200000.bin
#               COMPANION_AUDIO2=$COMPANION_DIR/AudioLX_0x04300000.bin
#               answer_is_bad=0
#               ;;
               * )
	       # Set multicom companion variable as sevaral config can be selected (we may not keep the default value which is in makefile)
	       export MULTICOM_COMPANION_CONFIG=0x15

	       # Set video firmwares
               COMPANION_VIDEO1=$COMPANION_DIR/VideoLX_7200A0V1_H264_VC1_MPEG2_MPEG4P2_V1_0x08000000.bin
               COMPANION_VIDEO2=$COMPANION_DIR/VideoLX_7200A0V2_H264_VC1_MPEG2_MPEG4P2_V2_0x08300000.bin
               answer_is_bad=0
               ;;
           esac
        # ........................................................
        ;;
        esac
# end while :
done

# ------ Store the choice in a file that will be used later by 'loadmods' when starting the board
case $DVD_BACKEND in
    "7100"|"7109" )
    echo $COMPANION_VIDEO >  MyFirmware.txt
    echo $COMPANION_AUDIO >> MyFirmware.txt
    # ........................................................
    ;;

    "7200" )
    echo $COMPANION_AUDIO1 >  MyFirmware.txt
    echo $COMPANION_VIDEO1 >> MyFirmware.txt
    echo $COMPANION_AUDIO2 >> MyFirmware.txt
    echo $COMPANION_VIDEO2 >> MyFirmware.txt
    # ........................................................
    ;;
esac
cp MyFirmware.txt $ROOTFS_DIR

# ------ Conclusion
unset answer_is_bad
unset answer_firmware
# Assumption : syntax of companion is VideoAAAAA_0xNNNN.bin where NNNN is the adress
case $DVD_BACKEND in
    "7100"|"7109" )
    companion_addr=0x`echo $COMPANION_VIDEO | cut -d "x" -f 2 | cut -d "." -f1`
    ;;

    "7200" )
    companion_addr="0x`echo $COMPANION_VIDEO1 | cut -d "x" -f 2 | cut -d "." -f1` and `echo $COMPANION_VIDEO2 | cut -d "x" -f 2 | cut -d "." -f1`"
    ;;
esac

echo "The following Companion will be used (video at address $companion_addr):"
unset companion_addr
unset COMPANION_DIR
case $DVD_BACKEND in
    "7100"|"7109" )
    unset COMPANION_AUDIO
    unset COMPANION_VIDEO
    ;;

    "7200" )
    unset COMPANION_AUDIO1
    unset COMPANION_VIDEO1
    unset COMPANION_AUDIO2
    unset COMPANION_VIDEO2
    ;;
esac
cat MyFirmware.txt

