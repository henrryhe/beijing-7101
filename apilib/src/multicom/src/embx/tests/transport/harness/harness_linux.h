#ifndef HARNESS_LINUX_H
#define HARNESS_LINUX_H

#define MMETEST_MAJOR_NUM 241
#define MMETEST_DEV_NAME  "mmetest"

typedef struct Terminate_s {
        /* IN */
        int code;
        int size;
        char* message;
        /* OUT */
} Terminate_t;

typedef enum IoCtlCmds_e {
        Terminate_c = 1
} IoCtlCmds_t;

#define MMETEST_IOCTL_MAGIC 'k'
#define MMETEST_IOX_TERMINATE  _IOWR(MMETEST_IOCTL_MAGIC, Terminate_c, Terminate_t)

#define DEBUG_NOTIFY_STR "   " DEBUG_FUNC_STR
#define DEBUG_ERROR_STR  "***" DEBUG_FUNC_STR

#if !defined MME_INFO_HARNESS
#define  MME_INFO_HARNESS 0
#endif

#define MME_Info(X,Y) {if (X) {harness_printf Y ;}}
#define EXIT_BUF_SIZE 256
#endif
