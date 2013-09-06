/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 1996

Source file name : report.h
Author :           Nick

    Contains the header file for the report functions,
    The actual reporting function, and a function to restrict
    the range of severities reported (NOTE Fatal and greater 
    messages are always reported). It is upto the caller to 
    insert file/line number information into the report. Process 
    information will be prepended to any report of severity error
    or greater.

Date        Modification                                    Name
----        ------------                                    ----
18-Dec-96   Created                                         nick
************************************************************************/
#ifndef  __REPORT_H__

#define  __REPORT_H__

typedef enum
    {
	severity_info	= 0,
	severity_error	= 100,
	severity_fatal	= 200
    } report_severity_t;

boolean report_init( void );

void report_restricted_severity_levels( int lower_restriction,
					int upper_restriction );

void do_report( report_severity_t   report_severity,
	     char 		*format, ... );

#endif   /* __REPORT_H__ */

