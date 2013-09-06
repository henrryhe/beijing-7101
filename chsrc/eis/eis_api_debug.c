/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_debug.c
  * 描述: 	实现debug输出的相关接口
  * 作者:	蒋庆洲
  * 时间:	2008-10-22
  * ===================================================================================
  */

#include "eis_api_define.h"
#include "eis_api_globe.h"
#include "eis_api_debug.h"
#include "stdarg.h"              	/* for argv argc  */

semaphore_t *p_EisDebugSmp=NULL; /*对打印接口增加保护防止函数访问重入*/
#if 1
#define __EIS_API_DEBUG_ENABLE__
#endif
int    gi_DebugControl = true;

#ifdef __EIS_API_DEBUG_ENABLE__
static char eis_string_p[1*1024] = "";
#endif

#ifdef __EIS_API_DEBUG_ENABLE__
static char eis_debug_string[512] = "";
#endif

void ipanel_EnableDebugControl (void)
{
   gi_DebugControl = true;
   
}
void ipanel_DisableDebugControl (void)
{
   gi_DebugControl = false;
}

/*
功能说明：
	iPanel Middleware 调用该函数向控制台输出调试信息，该函数的调用方法与C 标
	准库的printf 相同。实现时请使用开关量控制，以方便控制是否输出调试信息。
注意：
	iPanel MiddleWare 要求调试信息的输出缓冲至少为4K，否则有可能导致线程的异常。
参数说明：
	输入参数：
		fmt：格式化字符串。参数说明参照标准的printf 函数。
	输出参数：无
	返 回：
		>0: 实际输出的字符数；
		IPANEL_ERR: 函数异常返回。
  */
INT32_T ipanel_porting_dprintf( const CHAR_T *fmt , ...)
{
     if(gi_DebugControl == false)
     	{
			return 0;
     	}
#ifdef __EIS_API_DEBUG_ENABLE__
	{
		va_list Argument;
		if(p_EisDebugSmp==NULL)
		{
			p_EisDebugSmp = semaphore_create_fifo(1);
			if(p_EisDebugSmp==NULL)
				return 0;
		}
		
		if(strlen(fmt)>1023)
		{
			return 0;
		}
		 semaphore_wait(p_EisDebugSmp);
		/* Translate arguments into a character buffer */
		va_start ( Argument, fmt );
		vsprintf ( eis_string_p, fmt, Argument );
		va_end ( Argument );
	 
		/*STTBX_Print((String_p));*/
		sttbx_Print( "%s", eis_string_p );
		semaphore_signal ( p_EisDebugSmp );
	}
#endif
	return 0;
}
/*
功能说明：
	eis 接口debug信息调用
参数说明：
	输入参数：
		fmt：格式化字符串。参数说明参照标准的printf 函数。
	输出参数：无
	返 回：
		>0: 实际输出的字符数；
		IPANEL_ERR: 函数异常返回。
  */
INT32_T eis_report( const CHAR_T *fmt , ...)
{

#ifdef __EIS_API_DEBUG_ENABLE__
	va_list Argument;
	if(strlen(fmt)>511)
	{
		return 0;
	}
	
	if(p_EisDebugSmp==NULL)
	{
		p_EisDebugSmp = semaphore_create_fifo(1);
		if(p_EisDebugSmp==NULL)
			return 0;
	}
	semaphore_wait(p_EisDebugSmp);

	/* Translate arguments into a character buffer */
	va_start ( Argument, fmt );
	vsprintf ( eis_debug_string, fmt, Argument );
	va_end ( Argument );
 
	/*STTBX_Print((String_p));*/

	sttbx_Print( " %s", eis_debug_string );
	semaphore_signal ( p_EisDebugSmp );

#endif
	return 0;
}

/*--eof---------------------------------------------------------------------------------------------------*/

