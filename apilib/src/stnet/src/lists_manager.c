#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stddefs.h"
#include "stnet.h"
#include "net.h"
#include "net_recvtrans.h"

/*Todo - Replace STOS_TaskLock with semaphore */

U8 STNETi_AddToList(STNETi_Device_t* Device_p,STNETi_Handle_t* Handle_p)
{

    STNETi_Handle_t* List_p;
    U8 Index=0;

    STOS_TaskLock();

    if(Device_p->HandleList_p == NULL)
    {
        /*First Handle */
        Device_p->HandleList_p=Handle_p;
        Device_p->HandleList_p->Next_p=NULL;

    }
    else
    {
        List_p = Device_p->HandleList_p;
        for(Index=1; Index <= STNETi_MAX_HANDLES; Index++)
        {
            if((List_p->Next_p == NULL)||(List_p->Next_p->Instance != (List_p->Instance+1)))
            {
                /*Insert Handle here*/
                Handle_p->Next_p = List_p->Next_p;
                List_p->Next_p = Handle_p;

                break;
            }
            else
            {
                List_p = List_p->Next_p;
            }

        }
        if(Index == (STNETi_MAX_HANDLES+1))
            return 0;
    }

    Handle_p->Instance = Index+1;
    Device_p->Num_Open++;

    STOS_TaskUnlock();

    return Handle_p->Instance;


}

STNETi_Handle_t *STNETi_GetHandlefromList(STNETi_Device_t* Device_p,U8 Instance)
{

    STNETi_Handle_t* List_p;
    BOOL FoundHandle=FALSE;
    U8 Index;

    STOS_TaskLock();


    List_p = Device_p->HandleList_p;
    while (List_p != NULL)
    {
       if(List_p->Instance == Instance)
       {
          return  List_p;
       }
       List_p = List_p->Next_p;
    }

#if 0
    for(List_p = Device_p->HandleList_p; List_p != NULL; List_p = List_p->Next_p)
    {
        if(List_p->Instance == Instance)
        {
            FoundHandle = TRUE;
            break;
        }
    }
#endif

    STOS_TaskUnlock();
        return NULL;

}

STNETi_Handle_t * STNETi_RemovefromList(STNETi_Device_t* Device_p,U8 Instance)
{

    STNETi_Handle_t* List_p, *Prev_List_p;
    BOOL FoundHandle=FALSE;
    U8 Index;

    STOS_TaskLock();

    for(List_p = Device_p->HandleList_p; List_p != NULL; List_p = List_p->Next_p)
    {
        if(List_p->Instance == Instance)/*Found the Handle*/
        {
            Prev_List_p->Next_p= List_p->Next_p;
            Device_p->Num_Open--;
            FoundHandle = TRUE;
            break;
        }
    }

    STOS_TaskUnlock();

    if(FoundHandle)
        return List_p;
    else
        return NULL;

}

