/*********************************************************************************
 *      Copyright:  (C) 2023 LanLi 
 *                  All rights reserved.
 *
 *       Filename:  syslog.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(05/13/23)
 *         Author:  LanLi <2812093851@qq.com>
 *      ChangeLog:  1, Release initial version on "05/13/23 15:31:49"
 *                 
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#define LOG_FILE "syslog.text" 
#define MAX_LOG_FILE_SIZE (512 * 512) 
#define MAX_LOG_FILE_NUM 10 
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_ERROR 2

static FILE *log_file = NULL;
static int log_level = LOG_LEVEL_INFO;


void log_write(int level, const char *fmt, ...)
{
	time_t t;
	struct tm *tm;
	struct  tm result;
	va_list ap;
	char buf[1024];

	if (level < log_level) return;

	t = time(NULL);
	tm = localtime_r(&t,&result);

	switch (level) {
		case LOG_LEVEL_DEBUG:
			strcpy(buf, "[DEBUG] ");
			break;
		case LOG_LEVEL_INFO:
			strcpy(buf, "[INFO] ");
			break;
		case LOG_LEVEL_ERROR:
			strcpy(buf, "[ERROR] ");
			break;
	}
	strftime(buf + strlen(buf), sizeof(buf) - strlen(buf), "%Y-%m-%d %H:%M:%S", tm);
	sprintf(buf + strlen(buf), " [PID:%d]", getpid());

	va_start(ap, fmt);
	vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), fmt, ap);
	va_end(ap);

	printf("%s\n", buf);

	if (log_file == NULL)
    {
		log_file = fopen(LOG_FILE, "a+");
	}
	if (log_file != NULL)
    {
		fseek(log_file, 0, SEEK_END);
		if (ftell(log_file) > MAX_LOG_FILE_SIZE)
        {
			fclose(log_file);
			for (int i = MAX_LOG_FILE_NUM - 1; i >= 1; i--)
            {
				char old_file_name[256], new_file_name[256];
				if (i == MAX_LOG_FILE_NUM - 1)
                {
					snprintf(old_file_name, sizeof(old_file_name), "%s", LOG_FILE);
				}
                else
                {
					snprintf(old_file_name, sizeof(old_file_name), "%s.%d", LOG_FILE, i);
				}
				snprintf(new_file_name, sizeof(new_file_name), "%s.%d", LOG_FILE, i + 1);
				rename(old_file_name, new_file_name);
			}
			rename(LOG_FILE, LOG_FILE ".1");
			log_file = fopen(LOG_FILE, "a+");
		}
		if (log_file != NULL)
        {
			fprintf(log_file, "%s\n", buf);
			fflush(log_file);
		}
	}
}


void log_init(const char *config_file)
{
	log_level = LOG_LEVEL_DEBUG;
}

