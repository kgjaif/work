/********************************************************************************
 *      Copyright:  (C) 2023 lanli
 *                  All rights reserved.
 *
 *       Filename:  syslog.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(05/14/23)
 *         Author:  lanli
 *      ChangeLog:  1, Release initial version on "05/14/23 21:08:23"
 *                 
 ********************************************************************************/

#ifndef  _SYSLOG_H_
#define  _SYSLOG_H_

/*Description:This function used to record syslog*/
extern void log_write(int level, const char *fmt, ...);

/*Description:This function used to init syslog*/
extern void log_init(const char *config_file);

#endif /*----#ifndef _SYSLOG_H_ ------*/

