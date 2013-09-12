/*******************************************************************************

File name   : fields.h

Description :

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
 02 Sep 2002        Created                                   Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_DISPLAY_FIELDS_H
#define __VIDEO_DISPLAY_FIELDS_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Macros ---------------------------------------------------------- */

/* Returns the number of fields the picture must still display */
#define RemainingFieldsToDisplay(Picture)                     \
      ((Picture).Counters[VIDDISP_Data_p->DisplayRef].TopFieldCounter\
     + (Picture).Counters[VIDDISP_Data_p->DisplayRef].BottomFieldCounter\
     + (Picture).Counters[VIDDISP_Data_p->DisplayRef].RepeatFieldCounter\
     + ((Picture).Counters[VIDDISP_Data_p->DisplayRef].RepeatProgressiveCounter\
       * (((Picture).PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME ? 2 : 1) \
        + ((Picture).PictureInformation_p->ParsedPictureInformation.PictureGenericData.RepeatFirstField ? 1 : 0))))

/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t viddisp_DecrementFieldCounterBackward(
                        VIDDISP_Picture_t * const Picture_p,
                        VIDDISP_FieldDisplay_t * const NextFieldOnDisplay_p,
                        U32 Index);
ST_ErrorCode_t viddisp_DecrementFieldCounterForward(
                        VIDDISP_Picture_t * const Picture_p,
                        VIDDISP_FieldDisplay_t * const NextFieldOnDisplay_p,
                        U32 Index);
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_DISPLAY_FIELDS_H */

