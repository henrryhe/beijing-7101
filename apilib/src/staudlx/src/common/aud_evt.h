/*
 * Copyright (C) STMicroelectronics Ltd. 2002.
 *
 * All rights reserved.
 *
 * D program
 *
 */

#ifndef _AUDEVT_H_

    #define _AUDEVT_H_

    #ifndef ST_OSLINUX
        #include <stdlib.h>
        #include <stdio.h>
    #endif

    #include "stddefs.h"
    #include "stcommon.h"
    #include "stlite.h" /*include the proper os*/
    #include "stevt.h"
    #include "staudlx.h"
    #define STAUD_COMMON_EVENTS 1

    ST_ErrorCode_t  STAudEVT_Notify(STEVT_Handle_t Handle,STEVT_EventID_t EventID,void *EventData_p, U32 EventDataSize, 
                                    STAUD_Object_t ObjectID);

#endif // _AUDEVT_H_

