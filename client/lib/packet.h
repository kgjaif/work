#ifndef _PACKET_H_
#define _PACKET_H_


#include <string.h>
#include "syslog.h"
#include "ds18b20.h"

#define  DEVSN_LEN 10
typedef struct packet_s
{
    float temper;
    char  buf_t[32];
    char  devsn[DEVSN_LEN+1];
}packet_t;

extern int get_devsn(char *devsn,int size);

extern int get_data_time(char *buf_t,int size);

static inline int sample_temperature(packet_t *pack)
{
    if(!pack)
    {
        return -1;
    }
    memset(pack,0,sizeof(*pack));
    if(ds18b20_get_temperature(&pack->temper)<0)
    {
        return -2;
    }
    get_devsn(pack->devsn,sizeof(pack->devsn));
    get_data_time(pack->buf_t,sizeof(pack->buf_t));
}

extern int pack_data(packet_t *pack,char *buf,int size);

#endif /*-----#ifndef _PACKET_H_ -----*/
