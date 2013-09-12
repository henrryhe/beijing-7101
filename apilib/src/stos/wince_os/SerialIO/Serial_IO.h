#ifndef __SERIAL_IO__
#define __SERIAL_IO__

// Exported fonctions 
int	__cdecl	 wce_sdio_gets(char* str, int size);
int	__cdecl  wce_sdio_getchar(BOOL blocking );
int __cdecl  wce_sdio_printf(const char * format, ...);
BOOL __cdecl wce_sdio_puts(char* pInpBuffer);
BOOL __cdecl wce_sdio_putc(VOID* pInpBuffer);
void __cdecl wce_InitDebugSerial();

#endif