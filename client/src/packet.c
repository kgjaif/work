#include <time.h>
#include <stdio.h>
#include <string.h>
#include "ds18b20.h"
#include "syslog.h"
#include "packet.h"


#define LOG_LEVEL_INFO 1
extern void log_write(int level, const char *fmt, ...);

int get_devsn(char *devsn,int size)
{
    char sn[]="0001";
    memset(devsn,0,sizeof(devsn));
    snprintf(devsn,sizeof(devsn),"RPI%s",sn);
}

int get_data_time(char *buf_t,int size)
{
    struct tm *tm;
    struct tm result;
    time_t t;  
    t=time(NULL);
    tm = localtime_r(&t,&result);
    snprintf(buf_t,size,"%04d-%02d-%02d  %02d:%02d:%02d",
            tm->tm_year+1900, 
            tm->tm_mon+1,
            tm->tm_mday,
            tm->tm_hour,
            tm->tm_min,
            tm->tm_sec);

}

int pack_data(packet_t *pack,char *buf,int size)
{
    memset(buf,0,size);
    snprintf(buf,size,"%s,%f",pack->time,pack->temper,pack->devsn);
    log_write(LOG_LEVEL_INFO,"This is sample data:%s\n",buf);
}




