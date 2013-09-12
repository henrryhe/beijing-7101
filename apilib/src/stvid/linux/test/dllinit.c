#ifndef _WIN32
    /* empty file if not windows */
#else

/* dllinit.c -- Portable DLL initialization. 
   Copyright (C) 1998 Free Software Foundation, Inc.
   Contributed by Mumit Khan (khan@xraylith.wisc.edu).

   I've used DllMain as the DLL "main" since that's the most common 
   usage. MSVC and Mingw32 both default to DllMain as the standard
   callback from the linker entry point. Cygwin32 b19+ uses essentially 
   the same, albeit slightly differently implemented, scheme. Please
   see DECLARE_CYGWIN_DLL macro in <cygwin32/cygwin_dll.h> for more 
   info on how Cygwin32 uses the callback function.

   The real entry point is typically always defined by the runtime 
   library, and usually never overridden by (casual) user. What you can 
   override however is the callback routine that the entry point calls, 
   and this file provides such a callback function, DllMain.

   Mingw32: The default entry point for mingw32 is DllMainCRTStartup
   which is defined in libmingw32.a This in turn calls DllMain which is
   defined here. If not defined, there is a stub in libmingw32.a which
   does nothing.

   Cygwin32: The default entry point for cygwin32 b19 or newer is
   __cygwin32_dll_entry which is defined in libcygwin.a. This in turn
   calls the routine you supply to the DECLARE_CYGWIN_DLL (see below)
   and, for this example, I've chose DllMain to be consistent with all
   the other platforms.

   MSVC: MSVC runtime calls DllMain, just like Mingw32.

   Summary: If you need to do anything special in DllMain, just add it
   here. Otherwise, the default setup should be just fine for 99%+ of
   the time. I strongly suggest that you *not* change the entry point,
   but rather change DllMain as appropriate.

 */


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include<stdlib.h>
#define MAX_PATH_NAME _MAX_PATH 
#define MAX_FILE_NAME (_MAX_FNAME +  _MAX_EXT)
HINSTANCE m_handle;
BOOL APIENTRY DllMain (HINSTANCE hInst, DWORD reason, 
                       LPVOID reserved /* Not used. */ );
#define DBG_PRINT(fmt,args...) printf(fmt,##args)

#ifdef __CYGWIN32__

#include <cygwin32/cygwin_dll.h>
DECLARE_CYGWIN_DLL( DllMain );
/* save hInstance from DllMain */
HINSTANCE __hDllInstance_base;

#endif /* __CYGWIN32__ */

#if defined (ST_7200) && defined (NATIVE_CORE)
#include "tac_st40_native_wrapper_def.h"
#include "st40_esw_api.h"
static void * register_symbol(const char * symbol_name);
int (*tlm_sh4_create_task)(void * esw_wrapper,void *(*start_func)(void*),void * arg,
                        tlm_st40_task_struct_t *);
int (*tlm_sh4_delete_task)(void * esw_wrapper,tlm_st40_task_struct_t *,int task_finished);
tlm_st40_exception_handler_t (*tlm_sh4_register_interrupt_handler)(void * esw_wrapper,
                                tlm_st40_exception_handler_t handler,
                                tlm_st40_exception_arg_t * param);
tlm_st40_exception_handler_t (*tlm_sh4_register_trap_handler)(void * esw_wrapper,
                                tlm_st40_exception_handler_t handler,
                                tlm_st40_exception_arg_t * param);
#else /* (ST_7200) && (NATIVE_CORE) */
#include "st40_esw_api.h"
static void * register_symbol(const char * symbol_name);
int (*tlm_sh4_create_task)(void * esw_wrapper,void *(*start_func)(void*),void * arg,
                        st40_task_struct_t *);
int (*tlm_sh4_delete_task)(void * esw_wrapper,st40_task_struct_t *,int task_finished);
st40_exception_handler_t (*tlm_sh4_register_interrupt_handler)(void * esw_wrapper,
                                st40_exception_handler_t handler,
                                st40_exception_arg_t * param);
st40_exception_handler_t (*tlm_sh4_register_trap_handler)(void * esw_wrapper,
                                st40_exception_handler_t handler,
                                st40_exception_arg_t * param);
#endif /* (ST_7200) && (NATIVE_CORE) */

tlm_uint32_t (*tlm_sh4_status_set)(void * esw_wrapper,tlm_uint32_t mask);
tlm_uint32_t (*tlm_sh4_status_get)(void * esw_wrapper);
tlm_uint32_t (*tlm_sh4_get_intevt)(void * esw_wrapper);
tlm_uint32_t (*tlm_sh4_get_trapevt)(void * esw_wrapper);
tlm_uint32_t (*tlm_sh4_get_expevt)(void * esw_wrapper);
void (*tlm_sh4_trap)(void * esw_wrapper,int code);
void (*tlm_sh4_sleep)(void * esw_wrapper);

#if defined (ST_7200) && defined (NATIVE_CORE)
tlm_st40_task_struct_t * (*tlm_sh4_get_root_task)(void * esw_wrapper);
#else /* (ST_7200) && (NATIVE_CORE) */
st40_task_struct_t * (*tlm_sh4_get_root_task)(void * esw_wrapper);
#endif /* (ST_7200) && (NATIVE_CORE) */

tlm_uint32_t (*tlm_read32)(void * esw_wrapper,tlm_uint32_t addr);
void (*tlm_write32)(void * esw_wrapper,tlm_uint32_t addr,tlm_uint32_t data);
tlm_uint16_t (*tlm_read16)(void * esw_wrapper,tlm_uint32_t addr);
void (*tlm_write16)(void * esw_wrapper,tlm_uint32_t addr,tlm_uint16_t data);
tlm_uint8_t (*tlm_read8)(void * esw_wrapper,tlm_uint32_t addr);
void (*tlm_write8)(void * esw_wrapper,tlm_uint32_t addr,tlm_uint8_t data);
tlm_uint32_t (*sh4_tlm_to_virtual)(void * esw_wrapper,tlm_uint32_t);
tlm_uint32_t (*sh4_virtual_to_tlm)(void * esw_wrapper,tlm_uint32_t);


/*
 *----------------------------------------------------------------------
 *
 * DllMain --
 *
 *	This routine is called by the Mingw32, Cygwin32 or VC++ C run 
 *	time library init code, or the Borland DllEntryPoint routine. It 
 *	is responsible for initializing various dynamically loaded 
 *	libraries.
 *
 * Results:
 *      TRUE on sucess, FALSE on failure.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
extern char *argv[];
BOOL APIENTRY
DllMain (
	 HINSTANCE hInst /* Library instance handle. */ ,
	 DWORD reason /* Reason this function is being called. */ ,
	 LPVOID reserved /* Not used. */ )
{
  TCHAR ExecPath[1024];
  char *dynamic_lib;

  int lRes = GetModuleFileName(NULL,ExecPath,MAX_PATH);
  dynamic_lib=&ExecPath;
     
#ifdef __CYGWIN32__
  __hDllInstance_base = hInst;
#endif /* __CYGWIN32__ */

  switch (reason)
    {
    case DLL_PROCESS_ATTACH:
      /**Resolve the symbols with the main executable*/
    m_handle = LoadLibrary(dynamic_lib);
    if(m_handle == NULL) /* If error during dynamic load*/
    {
      fprintf(stderr,"\t%s: Error while loading library %s\n",__FUNCTION__, dynamic_lib);
      exit(1);
    }
    
#if defined (ST_7200) && defined (NATIVE_CORE)
tlm_sh4_create_task = (int (*)(void * esw_wrapper,void *(*start_func)(void*),\
		void * arg,tlm_st40_task_struct_t *))register_symbol("tlm_sh4_create_task");
tlm_sh4_delete_task = (int (*)(void * esw_wrapper,tlm_st40_task_struct_t *,int task_finished))register_symbol("tlm_sh4_delete_task");
tlm_sh4_register_interrupt_handler = (tlm_st40_exception_handler_t (*)(void * esw_wrapper,
                                tlm_st40_exception_handler_t handler,
                                tlm_st40_exception_arg_t * param))register_symbol("tlm_sh4_register_interrupt_handler");
tlm_sh4_register_trap_handler = (tlm_st40_exception_handler_t (*)(void * esw_wrapper,
                                tlm_st40_exception_handler_t handler,
                                tlm_st40_exception_arg_t * param))register_symbol("tlm_sh4_register_trap_handler");
#else /* (ST_7200) && (NATIVE_CORE) */
tlm_sh4_create_task = (int (*)(void * esw_wrapper,void *(*start_func)(void*),\
		void * arg,st40_task_struct_t *))register_symbol("tlm_sh4_create_task");
tlm_sh4_delete_task = (int (*)(void * esw_wrapper,st40_task_struct_t *,int task_finished))register_symbol("tlm_sh4_delete_task");
tlm_sh4_register_interrupt_handler = (st40_exception_handler_t (*)(void * esw_wrapper,
                                st40_exception_handler_t handler,
                                st40_exception_arg_t * param))register_symbol("tlm_sh4_register_interrupt_handler");
tlm_sh4_register_trap_handler = (st40_exception_handler_t (*)(void * esw_wrapper,
                                st40_exception_handler_t handler,
                                st40_exception_arg_t * param))register_symbol("tlm_sh4_register_trap_handler");
#endif /* (ST_7200) && (NATIVE_CORE) */
tlm_sh4_status_set = (tlm_uint32_t (*)(void * esw_wrapper,tlm_uint32_t mask))register_symbol("tlm_sh4_status_set");
tlm_sh4_status_get = (tlm_uint32_t (*)(void * esw_wrapper))register_symbol("tlm_sh4_status_get");
tlm_sh4_get_intevt = (tlm_uint32_t (*)(void * esw_wrapper))register_symbol("tlm_sh4_get_intevt");
tlm_sh4_get_trapevt = (tlm_uint32_t (*)(void * esw_wrapper))register_symbol("tlm_sh4_get_trapevt");
tlm_sh4_get_expevt = (tlm_uint32_t (*)(void * esw_wrapper))register_symbol("tlm_sh4_get_expevt");
tlm_sh4_trap = (void (*)(void * esw_wrapper,int code))register_symbol("tlm_sh4_trap");
tlm_sh4_sleep = (void (*)(void * esw_wrapper))register_symbol("tlm_sh4_sleep");

#if defined (ST_7200) && defined (NATIVE_CORE) 
tlm_sh4_get_root_task = (tlm_st40_task_struct_t * (*)(void * esw_wrapper))register_symbol("tlm_sh4_get_root_task");
#else /* (ST_7200) && (NATIVE_CORE) */
tlm_sh4_get_root_task = (st40_task_struct_t * (*)(void * esw_wrapper))register_symbol("tlm_sh4_get_root_task");
#endif /* (ST_7200) && (NATIVE_CORE) */
tlm_read32 = (tlm_uint32_t (*)(void * esw_wrapper,tlm_uint32_t addr))register_symbol("tlm_read32");
tlm_write32 = (void (*)(void * esw_wrapper,tlm_uint32_t addr,tlm_uint32_t data))register_symbol("tlm_write32");
tlm_read16 = (tlm_uint16_t (*)(void * esw_wrapper,tlm_uint32_t addr))register_symbol("tlm_read16");
tlm_write16 = (void (*)(void * esw_wrapper,tlm_uint32_t addr,tlm_uint16_t data))register_symbol("tlm_write16");
tlm_read8 = (tlm_uint8_t (*)(void * esw_wrapper,tlm_uint32_t addr))register_symbol("tlm_read8");
tlm_write8 = (void (*)(void * esw_wrapper,tlm_uint32_t addr,tlm_uint8_t data))register_symbol("tlm_write8");
sh4_tlm_to_virtual = (tlm_uint32_t (*)(void * esw_wrapper,tlm_uint32_t))register_symbol("sh4_tlm_to_virtual");
sh4_virtual_to_tlm = (tlm_uint32_t (*)(void * esw_wrapper,tlm_uint32_t))register_symbol("sh4_virtual_to_tlm");
DBG_PRINT("esw.so loaded successfully.\n");  
    break;

    case DLL_PROCESS_DETACH:
      break;

    case DLL_THREAD_ATTACH:
      break;

    case DLL_THREAD_DETACH:
      break;
    }
  return TRUE;
}

/**Register a symbol from dynamic library*/
static void * register_symbol(const char * symbol_name) {
    void * result = (void *)GetProcAddress(m_handle, symbol_name);
    if (result == NULL) {
      TCHAR  buf[80];
      LPVOID error_msg;
      DWORD  error_num = GetLastError();
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		    NULL,
		    error_num,
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		    (LPTSTR) &error_msg,
		    0,
		    NULL);
      
      sprintf(buf, "error %d: %s", error_num, error_msg);
    }
    return (result);
}
#endif


