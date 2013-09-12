/********************************************************************************
 *   Filename       : aud_defines.h                                         *
 *   Start              : 07.06.2007                                                                            *
 *   By                 : Nitin Jain                                            *
 *   Contact        : nitin-hpc.jain@st.com                                     *
 *   Description    : The file contains common structures required for audio driver module  *
 ******************************************************************************** */

#ifndef __AUDDEFINES_H__

    #define __AUDDEFINES_H__
    #define LPCMV_SAMPLES_PER_FRAME 640

    typedef struct STAud_LpcmStreamParams_s
    {
        /* Mark if there is some change in stream params */
        BOOL    ChangeSet;

        /* Possible streams params */
        U32     channels;
        U32     sampleBits;
        U32     sampleBitsGr2;
        U32     sampleRate;
        U32     sampleRateGr2;
        U32     bitShiftGroup2;
        U32     channelAssignment;

    } STAud_LpcmStreamParams_t;

#endif
