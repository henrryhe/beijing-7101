/*
  Name:key.h
  Description:header file of keyboard control section for changhong QAM5516 DBVC platform
  Authors:yxl
  Date          Remark
  2004-05-17    Created

*/



#ifndef KEY_H_
#define KEY_H_

typedef enum
{
    CURSOR_OFF,
    CURSOR_ON
} cursor_state_t;


/*export function*/


extern int InitKeyQueue(void);
extern int PutKeyIntoKeyQueue(int iKeyScanCode);
extern S8 IsKeyQueueEmpty(void);
extern int GetKeyFromKeyQueue(S8 WaitingSeconds);
extern int GetMilliKeyFromKeyQueue(int WaitingMilliSeconds);
extern int ReadKey(cursor_state_t cursorStatus, clock_t tclkWaitTime);
#define NO_KEY_PRESS    65535

#endif

