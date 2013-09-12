#include "aud_evt.h"


ST_ErrorCode_t  STAudEVT_Notify(STEVT_Handle_t Handle,STEVT_EventID_t EventID,void *EventData_p, U32 EventDataSize, STAUD_Object_t ObjectID)
{
    STAUD_Event_t EventData;
    EventData.ObjectID = ObjectID;
    #ifdef ST_OSLINUX
        #ifdef STAUD_COMMON_EVENTS
        {
            memset((U8 *)EventData.EventData, 0 , 16);
            if (EventData_p != NULL && EventDataSize != 0)
            {
                memcpy((U8 *)EventData.EventData,(U8 *)EventData_p, EventDataSize);
            }
            return(STEVT_NotifyWithSize(Handle,EventID,(void *)&EventData, sizeof(STAUD_Event_t)));
        }
        #else
            return(STEVT_NotifyWithSize(Handle,EventID,EventData_p,EventDataSize));
        #endif
    #else
        #ifdef STAUD_COMMON_EVENTS
        {
            EventData.EventData = EventData_p;
            return(STEVT_Notify(Handle,EventID,(void *)&EventData));
        }
        #else
            return(STEVT_Notify(Handle,EventID,EventData_p));
        #endif
    #endif  // endif ST_OSLINUX
}
