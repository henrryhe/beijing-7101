#define MAX_SUPPORT_FS_TYPE 13
#define MAX_LANGUAGE_ID 2
#define MAXPARTITION 20

typedef struct MOUNT_NAME
{
	char DiskMountPoint[32];
	char lvm[32];
	int gIsMountOk;
	int block;/*1block == 2K byte*/
}MOUNT_t;


extern ST_Partition_t *PICPartition;
extern int gPartitionNum;/*分区数*/
extern char gPVRMountRoot[32];/*PVR ROOT*/
extern int gCurMountIndex;/*MOUNT 区索引*/
extern MOUNT_t gDiskMountPoint[MAXPARTITION];
extern int gExt2;
/*extern semaphore_t*    Device_Exist_Protect;    sqzow20100617*/

