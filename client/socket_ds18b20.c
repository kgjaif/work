
#include <stdio.h>
#include <unistd.h> 
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <time.h>
#include <signal.h>
#include <libgen.h>

#include "syslog.h"
#include "ds18b20.h"
#include "packet.h"
#include "socket.h"
#include "database.h"

int sample_stop=0;

static void print_usage(char *program);
                                                      
void set_stop(int signum);


#define LOG_FILE "syslog.txt" 
#define MAX_LOG_FILE_SIZE (512 * 512) 
#define MAX_LOG_FILE_NUM 10 
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_ERROR 2
#define LOG_LEVEL_ALERT 3

static FILE *log_file = NULL;
static int log_level = LOG_LEVEL_INFO;

int main(int argc, char **argv)

{
	int                      ch;
	int                     rv=-1;
    int                     sockfd=-1;
	struct sockaddr_in      servaddr;
	char                   *servip=NULL;
	int                     port;
	char                    buf[512];
	char                   *domain=NULL;
	char                    buf_t[64];
	char                   *program=NULL;
    char                   *devsn=NULL;
    sqlite3                *db;
    packet_t                pack;
    socket_t                sock;
    time_t                  last_time;
    time_t                  cur_time;

	struct option           opts[]={
		{"ipaddr",required_argument,NULL,'i'},
		{"port",required_argument,NULL,'p'},
		{"doamin",required_argument,NULL,'d'},
		{"Help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};

	program=basename(argv[0]);


	while((ch=getopt_long(argc,argv,"i:p:d:h",opts,NULL))!=-1)
	{
		switch(ch)
		{

			case 'i':
			    servip=optarg;
				break;
			case 'p':
				port=atoi(optarg);
				break;
			case 'd':
				domain=optarg;
				break;
			case 'h':
				print_usage(argv[0]);
			default:
				log_write(LOG_LEVEL_ERROR,"Unknown return val: %d\n",ch);
		}
	}
           

	if(!port|!(!servip^!domain))
	{
		print_usage(argv[0]);
		return 0;
	}

	log_init("syslog.txt");

    if((socket_init(&sock,servip,port))<0)
    {
        log_write(LOG_LEVEL_ERROR,"Socket initialization failure!\n");
        return -1;
    }

    if(sql_open_database(&db)<0)
    {
        log_write(LOG_LEVEL_ERROR,"Create database failure!\n");
        return -2;
    }

    if(sql_create_table(db)<0)
    {
        log_write(LOG_LEVEL_ERROR,"Create table failure!\n");
        return -3;
    }

	last_time=time(NULL);

	while(1)
	{
        sample_stop=0;
        /*判断是否到达采样时间*/
        cur_time=time(NULL);  
        if(cur_time-last_time>=5)
        {
            /*直接采样*/
            if( sample_temperature(&pack)<0)
            {
                 log_write(LOG_LEVEL_ERROR,"Failed sample data\n");
                     return -4;
            }
            sample_stop=1;
          last_time=cur_time;  
        }
                
        if((socket_check(&sock))<0)
        {
            log_write(LOG_LEVEL_ERROR,"Socket check failure!\n");
                return -5;
        }

        if(!sock.connected)
        {
            if((socket_connect(&sock))<0)
            {
                log_write(LOG_LEVEL_ERROR,"Socket connect failure!\n");
                return -6;
            }
        }

        /*如果还是断开连接就存入数据库*/
        if(!sock.connected)
        {
            if(sample_stop)
            {
                if((sql_insert_data(db,buf,pack,sizeof(buf)))<0)
                {
                    log_write(LOG_LEVEL_ERROR,"Insert data into database failure\n");
                    return -7;
                }
            }
            continue;
        }

        /*socket连接正常就进行采样*/
        if(sample_stop)
        {
            memset(buf,0,sizeof(buf));
            pack_data(&pack,buf,sizeof(buf));
            if((socket_write(&sock,buf,strlen(buf)))<0)
            {
                printf("KKKKKK\n");
                printf("KKKK:%s\n",buf);
                if((sql_insert_data(db,buf,pack,sizeof(buf)))<0)
                {
                    printf("ooooooo:%s\n",buf);
                     log_write(LOG_LEVEL_ERROR,"Insert data into databasefailure\n");
                     return -8;
                }
                socket_close(&sock);
            }
        }
    }
        /*如果数据库中有数据就发送数据库的数据*/
        //memset(buf,0,sizeof(buf));
        //if()

	sqlite3_close(db);
	socket_close(&sock);
	return 0;
}

static void print_usage(char *program)
{
	printf("%s usage:\n",program);
	printf("-i(--ipaddr):sepcify server IP address\n");
	printf("-p(--port):sepcify server port\n");
	printf("-d(--domain):sepcify server domain\n");
	printf("-h(--help):print the help information\n");
	return ;
}


void set_stop(int signum)
{
	if(signum==SIGINT)
	{
		printf("exit\n");
	}
	sample_stop=1;
}
