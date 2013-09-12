#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "fdmatest.h"

void fdmatest(void)
{
    int fh;
    int ret;

    fh = open("/dev/stapi/fdmatest", O_RDWR);
    if (fh < 0)
    {
        perror("Couldn't open fdmatest");
        return;
    }

    ret = ioctl(fh, IO_STFDMA_StartTest);
    if (ret < 0)
        perror("Error starting test");
    else
        printf("Test started ok.\n");

    sleep(1);
    printf("Done.\n");

    close(fh);
}

int main(void)
{
    fdmatest();

    return 0;
}
