/* Wrapper from OS20 debug functions to OS21 */

#ifndef __os21debug_h
#define __os21debug_h

#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

#define debugmessage(message)           printf(message)
#define debugexit(status)               exit(status)
#define debuggets(buf, size)            gets(buf,size)
#define debuggetkey(buf)                1
#define debugcommand(cmd, result)       1
#define debugopen(filename, flags)      1
#define debugopen_fl(filename, flags)   1
#define debugclose(fd)                  close(fd)
#define debugread(fd, buf, size)        read(fd, buf, size)
#define debugwrite(fd, buf, size)       write(fd, buf, size)
#define debugflush(fd)                  flush(fd)
#define debugseek(fd, offset, whence)   seek(fd, offset, whence)
#define debugtell(fd)                   tell(fd)
#define debugtime()                     1
#define debuggetenv(name, buf, bufsize) 1
#define debugdisconnect()               1
#define debugbreak()                    1
#define debugconnected()                1
#define debuginformsenabled()           1
#define debugdrpccall(channel, block)   1
#define debugopendir(filename)          1
#define debugclosedir(fd)               1
#define debugreaddir(fd, dirent)        1
#define debugfstat(fd, buf)             1
#define debugstat(filename, buf)        1

#define DEBUG_FN_REENTERED -2
#define DEBUG_NOT_CONNECTED -3

/* inform signal - placed in Breg */
#define INFORM_FINISH          1
#define INFORM_DOINFORM        2
#define INFORM_SETMUTEXFN      3
#define INFORM_SETMUTEXDATA    4
#define INFORM_ISCONNECTED     5
#define INFORM_ISENABLED       6
#define INFORM_GETINFORMEX     7

#define DEBUGGER_CONNECTED     1
#define DEBUGGER_NOT_CONNECTED 2
#define INFORMS_ENABLED        3
#define INFORMS_DISABLED       4
#define INFORMEX_PRESENT       5

#define INFORM_CHAN_HOST       0
#define INFORM_CHAN_HTI        1

#ifdef __cplusplus
}
#endif

#endif
